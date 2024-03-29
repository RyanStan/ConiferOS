#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h>
#include "task/task.h"
#include "config.h"
#include "keyboard/keyboard.h"
#include "loader/formats/elf_file.h"

#define MAX_CMMD_ARG_LEN 32

enum executable_format {
        BINARY,
        ELF,
};

// Tracks a memory allocation made by a process
struct process_mem_allocation {
    /* ptr to the allocated memory.
     *
     * This is the address of the memory from the
     * kernel's view, and not the user program's view.
    */
    void *ptr;

    // Size of the allocation
    size_t size;
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
     */
    struct process_mem_allocation mem_allocs[PROCESS_MAX_ALLOCATIONS];

    /* The pointer to the memory that the process executable is loaded into.
     * If the process is instantiated from an ELF file, then elf_file will be set. 
     * If the process is instantiated from a binary executable, then binary_executable will be set.
     */
    union {
        void *binary_executable;
        struct elf_file *elf_file;
    };

    /* Pointer to the process's stack in memory 
     * Since kernel land executes with direct memory mapping (via page tables), this is the real physical address
     * of where the stack is loaded into memory.
     */
    void *stack_addr;

    /* The size of the binary_executable mapped to memory.
     * This is only valid if file format is BINARY.
     */
    uint32_t size;

    enum executable_format format;

    // TODO: I'm not entirely sure yet why the keyboard is tied to a process and not a task. Odd decision.
    // Well, in general, having this process struct in addition to tasks is odd.
    // Circular buffer
    struct keyboard_buffer
    {
        char buffer[KEYBOARD_BUFFER_SIZE];

        /* Head and tail must always be within the bounds of the buffer.
        * They should be updated by using their respective increment methods.
        * tail advances on write and head advances on read.
        */
        int head;
        int tail;
    } keyboard_buffer;

    // Number of arguments passed to this process when load process was called
    int argc;

    // Kernel space memory address of where the process's arguments (null terminated character arrays, aka strings) are stored.
    char **argv;

    // Allocated memory for storing command-line arguments. 
    // Mapped to COMMAND_LINE_ARG_VIRTUAL_ADDR in the process's address space.
    void *arg_block;
};
 
/* Loads the executable given by filename into memory, and generates a process
 * structure (with a task) that maps the executable.  
 * After calling, the address that process points to will now
 * be a pointer to the new process structure.  The process's 
 * tasks will also be ready to execute via task_exec (see task.h).
 */
int process_load(const char *filename, struct process **process);

/* Returns the process that is currently executing */
struct process *get_current_process();

/* Set the process that is currently executing.*/
void set_current_process(struct process *process);

/* Allocates memory from the kernel's heap and tracks the allocation in the process's mem_alloc array.
 * Returns 0 on error.
 *
 * See note in the implementation about why allocating memory from the kernel's heap for user process's
 * is a bad idea. This is a hacky approach.
 *
 */
void *process_malloc_syscall_handler(struct process *process, size_t size);

void process_free_syscall_handler(struct process *process, void *ptr);

// argv and the corresponding strings must be dynamically allocated on the kernel heap
int process_load_with_args(const char *filename, struct process **process, int argc, char *argv[]);

void put_argv_argc_in_stack_mem(void * process_stack_ptr, int argc, char * argv[]);

/* Frees the memory associated with the process
 * and unlinks it from the schedulable process list.
 * If the process was the currently executing process, then we
 * must run task_next() to execute the next task/process.
 */
int process_terminate(struct process *process);

#endif