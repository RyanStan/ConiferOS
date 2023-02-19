#include "task/task.h"
#include "status.h"
#include "config.h"
#include "memory/heap/kernel_heap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "kernel.h"
#include "string/string.h"

/*  This function is used to enter userland from kernel mode.
 * For each register in registers, it will set the real register with the corresponding value.
 * To do this, we make use of the iretd instruction.  More details are in the comments of the 
 * implementation in task.asm.
 */
void task_enter_userland(struct registers *registers);

/* Set all the unused segment registers to point at the user data segment. 
 * This must be called before switching back into userland
 */
void set_seg_regs_to_user_data();

/* The current task that is running */
struct task *current_task = 0;

/* Task doubly linked list */
struct task *task_list_head = 0;
struct task *task_list_tail = 0;

struct task *get_task_list_head()
{
    return task_list_head;
}

struct task *get_current_task()
{
    return current_task;
}

struct task *task_get_next()
{
    if (!current_task->next)
        return task_list_head;

    return current_task->next;
}

/* Remove task from the task doubly linked list */
static void task_list_remove(struct task *task)
{

    if (task == task_list_head && task == task_list_tail) {
        task_list_head = 0;
        task_list_tail = 0;
        current_task = 0;
    } else if (task == task_list_head) {
        task_list_head = task->next;
        task_list_head->prev = 0; 
    } else if (task == task_list_tail) {
        task_list_tail = task->prev;
        task_list_tail->next = 0;
    } else {
        task->prev->next = task->next;
    }

    if (task == current_task)
        current_task = task_get_next();

}

int task_free(struct task *task)
{
    free_page_tables(task->paging);
    task_list_remove(task);
    kfree(task);
    return 0;
}

/* Initialize a task structure by setting default values */
static int task_init(struct task *task, struct process *process) 
{
    memset(task, 0, sizeof(struct task));

    /* Create read only direct linear to physical mapping for entire 4 GB address space.
     * Accessible from ring 3 (user land + kernel land)
     *
     * We set PAGING_PRESENT (P) bit on every page table entry as opposed to mapping in pages as we need them.
     * This is to keep thing simple in the beginning.
     * TODO: Ideally, a new task should get empty page tables until they explicitly need to map in addresses
     *       (i.e. demand paging)
     */
    task->paging = init_page_tables(PAGING_USER_SUPERVISOR | PAGING_PRESENT);

    if (!task->paging)
        return -EIO;

    task->registers.ip = TASK_LOAD_VIRTUAL_ADDRESS;
    task->registers.ss = USER_DATA_SEGMENT;
    task->registers.cs = USER_CODE_SEGMENT;
    task->registers.esp = TASK_STACK_VIRT_ADDR;

    task->process = process;
    
    return 0;
}

struct task *task_new(struct process *process) 
{
    struct task *task = kzalloc(sizeof(struct task));
    if (!task)
        return ERROR(-ENOMEM);

    int rc = task_init(task, process);
    if (rc != STATUS_OK) {
        task_free(task);
        return ERROR(-1);
    }

    if (task_list_head == 0) {
        task_list_head = task;
        task_list_tail = task;
        current_task = task;
        return task;
    }

    task_list_tail->next = task;
    task->prev = task_list_tail;
    task_list_tail = task;
    task_list_tail->next = 0;

    return task;
}

int swap_curr_task_page_tables()
{
    set_seg_regs_to_user_data();
    paging_switch(current_task->paging);
    return 0;
}

int swap_task_page_tables(struct task *task)
{
    set_seg_regs_to_user_data();
    paging_switch(task->paging);
    return 0;
}

void task_exec(struct task *task)
{
    current_task = task;
    paging_switch(task->paging);
    task_enter_userland(&task->registers);
}

void task_current_save_state(struct interrupt_frame *frame)
{
    // Must be called from kernel land.
    // save the state of the task that was executing
    struct task *task = get_current_task();
    if (!task)
        panic("task_current_save_state: No current task!\n");

    task->registers.edi = frame->edi;
    task->registers.esi = frame->esi;
    task->registers.ebp = frame->ebp;
    task->registers.ebx = frame->ebx;
    task->registers.edx = frame->edx;
    task->registers.ecx = frame->ecx;
    task->registers.eax = frame->eax;
    task->registers.ip = frame->ip;
    task->registers.cs = frame->cs;
    task->registers.eflags = frame->eflags;
    task->registers.esp = frame->esp;
    task->registers.ss = frame->ss;
}

int copy_string_from_user_task(struct task *task, void *task_virt_addr, void *kernel_virt_addr, int max)
{
    if (max >= PAGING_PAGE_SIZE)
        return -EINVARG;

    /* Allocate memory that we can share between userland and kernel land.
     * The kernel can access the memory at address tmp, which is a physical address due to kernel linear page table mapping.
     * However, we will need to map the physical address tmp into the task's page tables,
     * so that we have a way to access the same physical memory from task land and kernel land. 
     */
    char *tmp = kzalloc(max);
    if (!tmp)
        return -ENOMEM;

    uint32_t *task_page_directory = task->paging->pgd;

    /* Get the page table entry in task's page tables that corresponds to tmp.
     * Since we will be temporarily remapping this page table entry, we will want to replace it
     * once we're done so that the task does not lose access to the virtual address mapping in case it was being used.
     */
    uint32_t old_entry = paging_get_pte(task_page_directory, tmp);

    /* Map the physical address tmp to virtual address tmp within the task's page tables. */
    paging_map_range(task->paging, tmp, tmp, 1, PAGING_READ_WRITE | PAGING_PRESENT | PAGING_USER_SUPERVISOR);
    
    /* Switch to the task's page tables.
     * Now, we can access the value at task_virt_addr.
     */
    paging_switch(task->paging);

    /* Copy the string at task_virt_addr to tmp. */
    strncpy(tmp, task_virt_addr, max);

    /* Switch back to kernel page tables */
    swap_kernel_page_tables();

    /* Restore task's page tables so that it can see whatever was at tmp before we overwrote it */
    if (paging_set(task_page_directory, tmp, old_entry) < 0) {
        kfree(tmp);
        return -EIO;
    }

    strncpy(kernel_virt_addr, tmp, max);
    return 0;
}

void *task_get_stack_item(struct task *task, int index)
{
    if (index < 0)
        panic("task_get_stack_item: index must be >= 0.\n");

    uint32_t *task_esp = (uint32_t*)task->registers.esp;

    /* Switch to the task's page tables.
     * We need to do this so that we can access the items
     * on the task's stack.
     */
    swap_task_page_tables(task);

    void *task_stack_item = (void*) task_esp[index];

    // Swap back in the kernel's page tables
    swap_kernel_page_tables();

    return task_stack_item;
}
