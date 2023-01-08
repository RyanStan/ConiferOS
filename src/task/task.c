#include "task/task.h"
#include "status.h"
#include "config.h"
#include "memory/heap/kernel_heap.h"
#include "memory/memory.h"
#include "kernel.h"


/*  This function is used to enter userland from kernel mode.
 * For each register in registers, it will set the real register with the corresponding value.
 * To do this, we make use of the iretd instruction.  More details are in the comments of the 
 * implementation in task.asm.
 */
void task_enter_userland(struct registers *registers);

/* Sets all segment registers to equal the current user data segment register
 * This must be called before switching back into userland.
 * TODO: function name doesn't make sense.  I would rather have a "set segment registers" function
 * Also, we set these registers in the task_return function instead....
 */
void user_registers();

/* The current task that is running */
struct task *current_task = 0;

/* Task doubly linked list */
struct task *task_list_head = 0;
struct task *task_list_tail = 0;

struct task *get_task_list_head()
{
    return task_list_head;
}

struct task *task_current()
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

/* Takes us out of kernel page directory and into current_task page directory 
 * TODO: we never call this, but it might be used in the future
 */
int task_page()
{
    user_registers();
    paging_switch(current_task->paging->pgd);
    return 0;
}

void task_exec(struct task *task)
{
    current_task = task;
    paging_switch(task->paging->pgd);
    task_enter_userland(&task->registers);
}