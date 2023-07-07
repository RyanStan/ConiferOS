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
    if (rc < 0)
        return rc;
    
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
    if (rc == -EIFORMAT)
        rc = process_load_binary_executable(filename, process);
    
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
 * TODO: mapping the entire elf file as writable isn't ideal.
 */
static int process_map_task_elf(struct process *process)
{
    struct elf_file *elf_file = process->elf_file;
    return paging_create_mapping(process->task->paging, paging_align_to_lower_page(elf_file->elf_virtual_addr_base), elf_file->elf_phys_addr_base,
                            paging_align_address(elf_file->elf_phys_addr_end), PAGING_PRESENT | PAGING_USER_SUPERVISOR | PAGING_READ_WRITE);
}

/* This function maps the process's executable memory (containing executable binary file or elf file)
 * and the stack into the page tables of the process's task.
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

/* Load the process into the given slot in processes array 
 * Loads the pointer to the new process struct into process parameter (hence why it's a double pointer).
 */
int process_load_for_slot(const char *filename, struct process **process, int pid)
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

    strncpy(_process->filename, filename, sizeof(_process->filename));
    _process->stack_addr = process_stack_ptr;
    _process->pid = pid;

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


int process_load(const char *filename, struct process **process)
{
    int pid = process_get_free_slot();
    if (pid < 0) {
        print("Could not load process.  Already at max processes.\n");
        return -EISTAKEN;
    }
        

    return process_load_for_slot(filename, process, pid);
}

void set_current_process(struct process *process)
{
    /* Since state is saved at the task-level
     * and this process_switch function is only called from kernel-land, 
     * there isn't any state for us to save here.
     */

    current_process = process;
}