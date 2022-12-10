#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "task/task.h"
#include "config.h"

enum executable_format {
        BINARY,
};

/* 
 * We're using this concept of processes to encapsulate tasks (i.e. threads).
 * Linux kernel uses the concept of lightweight threads which are still task_struct instances.
 * TODO: be more like the Linux Kernel and stick to task structures without introducing
 * this abstraction here.
*/
struct process {
    uint16_t pid;

    /* TODO: in Linux Kernel, a struct task doesn't directly contain a mapping
     * to a file.  Instead, the memory descriptor (mm_struct) contains fields that identify
     * crucial memory regions of the corresponding process like start_code/end_code, and
     * start_data/end_data (loaded in by exec functions).  I would prefer to eventually
     * structure this project like that.
     */
    char filename[MAX_FILE_PATH_CHARS];

    /* McCarthy's future intent is that a single process can encapsulate multiple tasks.
     * And for now, we're just implementing one task to keep things simple.  Thus, there
     * really a benefit to having the process abstraction right now. 
     */
    struct task *task;

    /* Kernel must keep track of the memory that a process allocates
     * in case the process doesn't free the memory itself.
     * This array contains the addresses of the allocated memory blocks.
     * TODO: this should ideally be unlimited.
     */
    void *mem_allocs[PROCESS_MAX_ALLOCATIONS];

    /* The pointer to the memory that the process executable is loaded into.
     * This assumes we're loading a program as binary (i.e. no ELF sections like text, data, etc.).
     * Since kernel land executes with direct memory mapping (via page tables), this is the real physical address
     * of where the text is loaded into memory.
     */
    void *executable_memory_addr;

    /* Pointer to the process's stack in memory 
     * Since kernel land executes with direct memory mapping (via page tables), this is the real physical address
     * of where the stack is loaded into memory.
     */
    void *stack_addr;

    /* The size of the executable mapped to memory at executable_memory_addr*/
    uint32_t size;

    enum executable_format format;
};

#endif