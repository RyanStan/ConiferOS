#include "task/process.h"
#include "task/task.h"
#include "memory/memory.h"
#include "memory/heap/kernel_heap.h"
#include "fs/file.h"
#include "status.h"
#include "memory/paging/paging.h"
#include "print/print.h"
#include "string/string.h"
#include "kernel.h"
#include "loader/formats/elf_file.h"
#include "loader/formats/elf.h"
#include <stdbool.h>
#include "process.h"
#include "config.h"

/* The current process that is running */
struct process *current_process = 0;

/* List of processes that could be executed.
 * Each index corresponds to a process's pid.
 * TODO: turn this into linked list for infinite processes.
 */
static struct process *processes[MAX_PROCESSES];

static void process_init(struct process *process)
{
    memset(process, 0, sizeof(struct process));
}

struct process *get_current_process()
{
    return current_process;
}

struct process *process_get(int pid)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
        return NULL;

    return processes[pid];
}

// Load the data in filename into memory and points the process's binary_executable field to this data
static int process_load_binary_executable(const char *filename, struct process *process)
{
    int fd = fopen(filename, "r");
    if (fd < 0)
        return -EIO;

    /* Get the file size */
    struct file_stat stat;
    int rc = fstat(fd, &stat);
    if (rc < 0)
        goto out;

    /* We will load the file at filename into virtual memory at binary_executable */
    void *binary_executable = kzalloc(stat.filesize);
    if (!binary_executable) {
        rc = -ENOMEM;
        goto out;
    }

    if ((rc = fread(binary_executable, stat.filesize, 1, fd)) < 0)
        goto out;
    
    process->binary_executable = binary_executable;
    process->size = stat.filesize;
    process->format = BINARY;

out:
    fclose(fd);
    return rc;
}

static int process_load_elf_executable(const char *filename, struct process *process)
{
    struct elf_file *elf_file = 0;
    int rc = elf_file_init(filename, &elf_file);
    if (rc < 0) {
        print("process_load_elf_executable: error initializing elf_file\n");
        return rc;
    }
    
    process->format = ELF;
    process->elf_file = elf_file;
    return 0;
}

/* Load the data in filename into memory and updates the process structure accordingly.
 * Will attempt to parse the file as an ELF executable, and if that fails, then a binary executable.
 *
 * Note: this function does not load the executable into the page tables of the process's current task. That is done by process_map_task_memory.
 */
static int process_load_data(const char *filename, struct process *process)
{
    int rc = process_load_elf_executable(filename, process);
    if (rc == -EIFORMAT) {
        rc = process_load_binary_executable(filename, process);
    }
    return rc;
}

/* Maps the process's binary_executable into the page tables of the process's task 
 * at address TASK_LOAD_VIRTUAL_ADDRESS.
 */
static int process_map_task_binary(struct process *process)
{
    return paging_create_mapping(process->task->paging, (void*)TASK_LOAD_VIRTUAL_ADDRESS, 
                                    process->binary_executable, 
                                    paging_align_address(process->binary_executable + process->size),
                                    PAGING_PRESENT | PAGING_USER_SUPERVISOR | PAGING_READ_WRITE);
}

/* Maps the process's ELF file's loadable segments into the page tables of the process's tasks.
 * 
 */
static int process_map_task_elf(struct process *process)
{
    struct elf_file *elf_file = process->elf_file;
    int rc = 0;

    // Loop through the program headers
    struct elf32_ehdr *elf32_ehdr = elf_get_ehdr(elf_file);
    struct elf32_phdr *phdr_table = elf_get_phdr_table(elf32_ehdr);
    
    for (int i = 0; i < elf32_ehdr->e_phnum; i++) {
        struct elf32_phdr *phdr = &phdr_table[i];
        void *phdr_phys_addr = elf_file_get_segment_phys_addr(elf_file, phdr);
        int pflags = PAGING_PRESENT | PAGING_USER_SUPERVISOR;
        if (phdr->p_flags & PF_W) {
            pflags |= PAGING_READ_WRITE;
        }
        
        rc = paging_create_mapping(process->task->paging, paging_align_to_lower_page((void *)phdr->p_vaddr), paging_align_to_lower_page((void *)phdr_phys_addr),
                            paging_align_address(phdr_phys_addr+phdr->p_memsz), pflags);
        if (rc < 0)
            break;
    }

    return rc;
}

/* This function maps the process's executable memory (containing executable binary file or elf file), 
 * the stack, and command-line arguments, into the page tables of the process's task.
 *
 */
int process_map_task_memory(struct process *process)
{
    // Map the stack's physical address into task's page tables
    int rc = paging_create_mapping(process->task->paging, (void*)TASK_STACK_VIRT_ADDR_END, process->stack_addr,
                                    paging_align_address(process->stack_addr + TASK_STACK_SIZE), 
                                    PAGING_PRESENT | PAGING_USER_SUPERVISOR | PAGING_READ_WRITE);
    if (rc < 0)
        return rc;
    
    int len_arg_block = MAX_CMMD_ARG_LEN * MAX_NUM_ARGS + sizeof(char*) * MAX_NUM_ARGS;
    rc = paging_create_mapping(process->task->paging, (void *)COMMAND_LINE_ARG_VIRTUAL_ADDR, process->arg_block,
                                    paging_align_address(process->arg_block + len_arg_block),
                                    PAGING_PRESENT | PAGING_USER_SUPERVISOR | PAGING_READ_WRITE);
    if (rc < 0)
        return rc;

    // Map the executable's physical address into task's page tables.
    switch (process->format)
    {
        case ELF:
            rc = process_map_task_elf(process);
            break;
        case BINARY:
            rc = process_map_task_binary(process);
            break;
        default:
            panic("ERROR process_map_task_memory: unexpected executable format\n");
            break;
    }
    return rc;
}

/* Free the kernel heap memory allocated for process */
void process_free(struct process *process)
{
    // TODO: make sure to free any left over memory_allocations as well

    switch (process->format)
    {
        case ELF:
            if (process->elf_file) {
                elf_file_close(process->elf_file);
            }
            break;
        case BINARY:
            if (process->binary_executable) {
                kfree(process->binary_executable);
            }
            break;
        default:
            break;
    }

    if (process->stack_addr)
        kfree(process->stack_addr);
}

// Given an address from the arg block of memory, translate it to its userspace equivalent.
// For example, if the kernel allocates memory at 0x0F for a process, but 
// maps arg block memory to 0xA in user space, then the kernel will translate 0x0F to 0x0A.
void* translate_arg_block_addr_to_userspace_addr(void *addr, void *arg_block) 
{
    if ((uint32_t)addr < (uint32_t)arg_block) {
        // Invalid input
        return 0;
    }

    uint32_t diff = (uint32_t)addr - (uint32_t)arg_block;
    return (void *)((uint32_t)COMMAND_LINE_ARG_VIRTUAL_ADDR + diff);
}

// Load a process's stack with argc, argv, and load the argument strings into the correct places
// in the stack.
void load_process_stack_memory_with_args(void *process_stack_ptr, int argc, char *argv[])
{
    // Since the stack grows downward, a process's esp register will point
    // at the end of the allocated stack memory after the process is initialized by the kernel.
    // This is why we must put the arguments at the end of the stack memory.
    // TODO: we'll have to put the esp register below all of this now....
    // maybe I can shove all the argv stuff from the beginning of the stack memory, and not
    // change the way this works!

    void *argc_location = (char *)process_stack_ptr + TASK_STACK_SIZE - 4;
    *(int *)argc_location = argc;
    //void *argv_location = (char *)process_stack_ptr + TASK_STACK_SIZE - 8;

    // TODO: put argv and argc at the beginning of process_stack_ptr, not the end! meh that might mess things up?
    // Idk. Maybe I do want to allocate memory,and just hardcode argv

    // This is the array of char pointers, each of which is a pointer to an argument
    char **argv_char_array_location = (char **)((char *)process_stack_ptr + TASK_STACK_SIZE - 12); // this will be the value of argv

    //*(char **)argv_location = (void *)translate_stack_addr_to_userspace_addr((void *)argv_char_array_location);
    
    // This is a pointer to the first args string (character array)
    char *arg_strings_location = (char *)(argv_char_array_location + MAX_CMMD_ARG_LEN + 1); // The 1 is a safety bumper

    /* This code is complicated, here's a break-down.
     *
     * args_string_location is a pointer to the beginning of memory where we'll store 
     * the argument strings, which are sequences of null-terminated chars. Thus, args_string_location is a pointer
     * to a character.
     * 
     * argv_char_array_location is a pointer to the memory where we store the array of argument string pointers.
     * Argv will point to this memory. Thus, argv will take on the value argv_char_array_location.
     * 
     * Here, we copy the strings referenced by argv[i] into args_string_location. 
     * Then, we update argv_char_array_location with the address of the beginning of the string.
     * 
     * We have to make sure that the addresses in argv and argv_char_array are translated to
     * the userspace equivalent, since the stack memory is mapped into TASK_STACK_VIRT_ADDR.
     * 
     * TODO: walk through this code in debugger and make sure I got it right, before implementing translation
     * function
     *
    */
    for (int i = 0; i < argc; i++) {
        int arg_len = strnlen(argv[i], MAX_CMMD_ARG_LEN);
        strcpy(arg_strings_location, argv[i]);
        //argv_char_array_location[i] = (char*)translate_stack_addr_to_userspace_addr(arg_strings_location);
        arg_strings_location += arg_len + 5; // The 5 is a safety bumper 
    }

    return;
}

// Given a pointer to a process's stack, which is of size TASK_STACK_SIZE bytes,
// place argc and argv into the end of that memory, so that a process may access
// those values through its stack pointer.
void put_argv_argc_in_stack_mem(void *process_stack_ptr, int argc, char *argv[])
{
    // Since the stack grows downward, a process's esp register will point
    // at the end of the allocated stack memory after the process is initialized by the kernel.
    // This is why we must put the arguments at the end of the stack memory.

    void *process_stack_flipped = (char *)process_stack_ptr + TASK_STACK_SIZE - 4;
    *(int *)process_stack_flipped = argc;

    // Put argc on the program's stack. --> print(*(int*)process_stack_ptr) ... does ths work?
    process_stack_flipped = (char *)process_stack_ptr + TASK_STACK_SIZE - 8;
    *(char **)process_stack_flipped = (char *)argv;
}

void load_arg_block(void *arg_block, int argc, char *argv[])
{
   char **user_argv = (char **)arg_block;
   char *arg_strings = (char *)(user_argv + MAX_NUM_ARGS); // incrementing arg strings should increment address by four bytes per unit (user arg points to points)

    for (int i = 0; i < argc; i++) {
        //int arg_len = strnlen(argv[i], MAX_CMMD_ARG_LEN);
        print(argv[i]);
        print("  -->processed that arg \n");
        strcpy(arg_strings, argv[i]);
        print(arg_strings);
        print("  -->arg strings had this value \n"); // I hvae the right values.... hmm I'll need the debugger to work. save it for next week
        user_argv[i] = (char*)translate_arg_block_addr_to_userspace_addr(arg_strings, arg_block);
        arg_strings += MAX_CMMD_ARG_LEN;
    }
}

/* Load the process into the given slot in processes array 
 * Loads the pointer to the new process struct into process parameter (hence why it's a double pointer).
 */
int process_load_for_slot(const char *filename, struct process **process, int pid, int argc, char *argv[])
{
    int rc = 0;

    /* Make sure that a process with pid doesn't already exist */
    if (process_get(pid) != NULL) {
        rc = -EISTAKEN;
        goto out;
    }

    struct process *_process = kzalloc(sizeof(struct process));
    if (!_process) {
        rc = -ENOMEM;
        goto out;
    }

    process_init(_process);
    rc = process_load_data(filename, _process);
    if (rc < 0)
        goto out;

    void *process_stack_ptr = kzalloc(TASK_STACK_SIZE);
    if (!process_stack_ptr) {
        rc = -ENOMEM;
        goto out;
    }

    //argc = 5; hardcoding just so that I can test accessing this value in user space.
    if (argc == 0) {
        argv = 0;
    }

    //load_process_stack_memory_with_args(process_stack_ptr, argc, argv);
    put_argv_argc_in_stack_mem(process_stack_ptr, argc, /*argv*/(void *)COMMAND_LINE_ARG_VIRTUAL_ADDR);
    _process->argc = argc;
    _process->argv = argv;

    strncpy(_process->filename, filename, sizeof(_process->filename));
    _process->stack_addr = process_stack_ptr;
    _process->pid = pid;

    _process->arg_block = kzalloc(MAX_CMMD_ARG_LEN * MAX_NUM_ARGS + sizeof(char*) * MAX_NUM_ARGS);
    if (!_process->arg_block) {
        rc = -ENOMEM;
        goto out;
    }
    load_arg_block(_process->arg_block, _process->argc, _process->argv);

    /* Create a task */
    struct task *task = task_new(_process);
    if (task < 0) {
        rc = ERROR_I(task);
        goto out;
    }
    _process->task = task;

    /* Configure the process's task's page tables */
    rc = process_map_task_memory(_process);
    if (rc < 0)
        goto out;

    *process = _process;

    processes[pid] = _process;

out:
    if (rc < 0) {
        if (_process && _process->task)
            task_free(_process->task);

        process_free(_process);
    }
    return rc;
}

/* Returns the index of the next empty slot (pid) in processes array */
int process_get_free_slot()
{
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] == 0)
            return i;
    }

    return -EISTAKEN;
}

int process_load_with_args(const char *filename, struct process **process, int argc, char *argv[])
{
    int pid = process_get_free_slot();
    if (pid < 0) {
        print("Could not load process.  Already at max processes.\n");
        return -EISTAKEN;
    }

    return process_load_for_slot(filename, process, pid, argc, argv);
}

int process_load(const char *filename, struct process **process)
{
    int pid = process_get_free_slot();
    if (pid < 0) {
        print("Could not load process.  Already at max processes.\n");
        return -EISTAKEN;
    }
        
    return process_load_for_slot(filename, process, pid, 0, NULL);
}

void set_current_process(struct process *process)
{
    /* Since state is saved at the task-level
     * and this process_switch function is only called from kernel-land, 
     * there isn't any state for us to save here.
     */

    current_process = process;
}

/* Find a slot within the process's memory allocations array that does not point to
 * memory allocated by a user process yet.
 * Returns the index of the slot, or < 0 on errors.
 */
static int process_get_free_memory_allocation_slot(struct process *process)
{
    for (int i = 0; i < PROCESS_MAX_ALLOCATIONS; i++) {
        if (process->mem_allocs[i].ptr == 0) {
            // the memory allocation slot is free
            return i;
        }
    }
    return -ENOMEM;
}

void *process_malloc_syscall_handler(struct process *process, size_t size)
{
    /* TODO [RyanStan 09-19-23]  Sharing a memory heap between user processes and the kernel
     * is not a good idea. You could have a user process fragment the heap and cause
     * problems in the kernel.
     */
    void *ptr = kzalloc(size);
    if (!ptr) {
        kfree(ptr);
        return 0;
    }

    int index = process_get_free_memory_allocation_slot(process);
    if (index < 0) {
        kfree(ptr);
        return 0;
    }

    /* Create a 1:1 mapping from the physical
     * memory address returned by kzalloc to the virtual address in the process's page tables.
     *
     * TODO [RyanStan 09-22-23] This 1:1 mapping could cause problems - how do we know
     * that something else won't be mapped into that virtual address space at the address ptr? 
     * We should keep track of the memory regions that have been mapped into the task's address space.
     */
    int rc = paging_create_mapping(process->task->paging, (void*)ptr, ptr,
                                    paging_align_address(ptr + size), 
                                    PAGING_PRESENT | PAGING_USER_SUPERVISOR | PAGING_READ_WRITE);
    if (rc < 0) {
        kfree(ptr);
        return 0;
    }

    process->mem_allocs[index].ptr = ptr;
    process->mem_allocs[index].size = size;
    return ptr;
}

// Loop through the process's memory allocations and return the process's memory allocation
// that begins at addr
static struct process_mem_allocation *get_process_allocation(struct process *process, void *addr)
{
    for (int i = 0; i < PROCESS_MAX_ALLOCATIONS; i++) {
        if (process->mem_allocs[i].ptr == addr) {
            return &process->mem_allocs[i];
        }
    }

    return 0;
}

void process_free_syscall_handler(struct process *process, void *ptr) 
{

    // Invalidate the process's page table entries for ptr
    struct process_mem_allocation *allocation = get_process_allocation(process, ptr);
    if (!allocation) {
        // If we did not find a memory allocation associated with ptr, then we should not
        // do anything. This prevents us from freeing arbitrary memory addresses that user programs
        // pass to us.
        return;
    }

    // Invalidate the process's page table entries which map ptr to memory.
    // This prevents the process from writing to memory through ptr after ptr has been freed.
    //
    // TODO [RyanStan 09-22-23] Being able to read the memory still isn't ideal, but we initialize
    // the entire's task's page tables to readable with a direct mapping to physical memory anyways. 
    int rc = paging_create_mapping(process->task->paging, allocation->ptr, allocation->ptr,
                                paging_align_address(allocation->ptr + allocation->size), 
                                PAGING_USER_SUPERVISOR | PAGING_PRESENT);
    if (rc < 0) {
        return;
    }

    kfree(ptr);

    allocation->ptr = 0;
    allocation->size = 0;
}