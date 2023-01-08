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

    /* We will load the file at filename into virtual memory at executable_memory_addr */
    void *executable_memory_addr = kzalloc(stat.filesize);
    if (!executable_memory_addr) {
        rc = -ENOMEM;
        goto out;
    }

    if ((rc = fread(executable_memory_addr, stat.filesize, 1, fd)) < 0)
        goto out;
    
    process->executable_memory_addr = executable_memory_addr;
    process->size = stat.filesize;
    process->format = BINARY;

out:
    fclose(fd);
    return rc;
}

/* Load the data (binary executable) in the file at filename into memory */
static int process_load_data(const char *filename, struct process *process)
{
    /* TODO: in the future, we want to support ELF files as well */
    return process_load_binary_executable(filename, process);
}

int process_map_task_binary(struct process *process)
{
    return paging_create_mapping(process->task->paging, (void*)TASK_LOAD_VIRTUAL_ADDRESS, 
                                    process->executable_memory_addr, 
                                    paging_align_address(process->executable_memory_addr + process->size),
                                    PAGING_PRESENT | PAGING_USER_SUPERVISOR | PAGING_READ_WRITE);
}

/* This function maps the process's executable memory (containing executable binary file)
 * and the stack into the page tables of the process's task.
 *
 * TODO: support ELF.
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
    return process_map_task_binary(process);
}

/* Free the kernel heap memory allocated for process */
void process_free(struct process *process)
{
    // TODO: make sure to free any left over memory_allocations as well

    if (process->executable_memory_addr)
        kfree(process->executable_memory_addr);

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