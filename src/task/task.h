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

/* Swaps out kernel page tables and swaps in current_task page tables 
 * It also sets ds, es, fs, gs segment registers to the user data segment.
 */
int swap_curr_task_page_tables();

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

#endif