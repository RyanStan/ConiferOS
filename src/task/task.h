#ifndef TASK_H
#define TASK_H

#include "config.h"
#include "memory/paging/paging.h"

struct process;

/* Hardware context of the task */
struct registers {
    uint32_t edi;       // destination index register
    uint32_t esi;       // source index register
    uint32_t ebp;       // stack base pointer register
    uint32_t ebx;       // general register
    uint32_t edx;       // general register     
    uint32_t ecx;       // general register
    uint32_t eax;       // general register

    uint32_t ip;        // instruction pointer. Last address that was executing before task switch.
    uint32_t cs;        // code segment
    uint32_t eflags;    // state of the processor
    uint32_t esp;       // stack pointer
    uint32_t ss;        // stack segment
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
struct task *task_current();

/* Return the next task that is scheduled to execute */
struct task *task_get_next();

/* Free the memory associated with task from the kernel heap */
int task_free(struct task *task);


#endif