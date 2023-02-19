#ifndef TASK_H
#define TASK_H

#include "config.h"
#include "memory/paging/paging.h"
#include "idt/idt.h"

struct process;

/* Hardware context of the task */
struct registers {
    uint32_t edi;       // destination index register       [&registers]
    uint32_t esi;       // source index register            [&registers+4]
    uint32_t ebp;       // stack base pointer register      [&regisers+8]
    uint32_t ebx;       // general register                 [&registers+12]
    uint32_t edx;       // general register                 [&registers+16]
    uint32_t ecx;       // general register                 [&registers+20]
    uint32_t eax;       // general register                 [&registers+24]

    uint32_t ip;        // instruction pointer. Last address that was executing before task switch. [&registers+28]
    uint32_t cs;        // code segment                     [&registers+32]
    uint32_t eflags;    // state of the processor           [&registers+36]
    uint32_t esp;       // stack pointer                    [&registers+40]
    uint32_t ss;        // stack segment                    [&registers+44]
};


/* A task is the unit of scheduling that our kernel understands. 
 * We schedule tasks, not processes.
 */
struct task {
    /* Contains the pointer to the pgd for the task. Maps 4 GB of linear (virtual) to physical memory. */
    struct paging_desc *paging;

    /* Stores the hardware context (register state) of the task when it's not running.  
     * Linux kernel does this in thread_info.
     */
    struct registers registers;

    /* The process that owns this task */
    struct process *process;

    struct task *next;
    struct task *prev;

};

/* Creates a new task structure and returns a pointer to it.  Initializes
 * default values for the new task and adds it to the task list.
 * process is the process that will own the task.
 * Returns new task on success or < 0 on failure.
 */
struct task *task_new(struct process *process);

/* Return the currently executing task */
struct task *get_current_task();

/* Returns the head task in the task list */
struct task *get_task_list_head();

/* Return the next task that is scheduled to execute */
struct task *task_get_next();

/* Free the memory associated with task from the kernel heap */
int task_free(struct task *task);

/* Swaps out kernel page tables and swaps in current_task page tables.
 * Also set ds, es, fs, gs segment registers to the user data segment.
 */
int swap_curr_task_page_tables();

/* Swaps out kernel page tables and swaps in task's page tables.
 * Also set ds, es, fs, gs segment registers to the user data segment.
 */
int swap_task_page_tables(struct task *task);

/* Executes the given task.
 * This will switch the current page tables and
 * restore registers to the state of task's registers. Finally,
 * it will drop into userland.
 * 
 * The parameter task must be in the task list (task.c)
 * before calling this function.  That can be achieved by first calling
 * process_load (see process.h).
 */
void task_exec(struct task *task);

/* Saves the register values in frame into the current task's registers object.  
 * This allows us to restore the state of the registers when that task resumes execution.
 */
void task_current_save_state(struct interrupt_frame *frame);

/* Copy a string from userland (task's address space) to the kernel's address space.
 * This function must be called from kernel land.
 * This function relies on the fact that the kernel page table's linearly map the whole physical address space.
 *
 *  task_virt_addr - A userland virtual address that is mapped to some value via task's page tables.  
 *                   Kernel mode code cannot directly see the value at this address since it is mapped to a physical address
 *                   by task's page tables, and not the kernel's page tables.
 * 
 *  kernel_virt_addr - Kernel virtual address (mapped by kernel page tables).  We map the userland value at task_virt_addr
 *                     into kernel_virt_addr so that the kernel can access the value.
 * 
 *   max - The maximum length of the string that we are copying from userland to kernel land.
 *
 */
int copy_string_from_user_task(struct task *task, void *task_virt_addr, void *kernel_virt_addr, int max);

/* Retrieve items from the task's stack.  
 * This function switches into the task's page tables,
 * pulls the stack item at index using the task's saved esp register value,
 * and then swaps back in the kernel's page tables.
 * The value is returned as a void* type, because the value will be
 * 4 bytes but we won't know it's type.
 */
void *task_get_stack_item(struct task *task, int index);

#endif