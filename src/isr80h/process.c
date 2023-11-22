#include "process.h"
#include "task/task.h"
#include "task/process.h"
#include "print/print.h"
#include "status.h"
#include "memory/paging/paging.h"
#include "kernel.h"
#include "string/string.h"
#include "memory/heap/kernel_heap.h"
#include "config.h"

int copy_argv_pointers_from_user_task(struct task *task, char *argv_usr_addr[], char *kernel_argv[], int argc);

void *isr80h_command_6_execve(struct interrupt_frame *frame)
{
    struct task *current_task = get_current_task();

    int argc = (int) task_get_stack_item(current_task, 0);
    void *argv_usr_addr = task_get_stack_item(current_task, 1);
    void *filename_usr_addr = task_get_stack_item(current_task, 2);

    char filename[MAX_FILE_PATH_CHARS];
    copy_string_from_user_task(current_task, filename_usr_addr, filename, MAX_FILE_PATH_CHARS);

    // Retrieve the contents of the argv array from user space, and store it in argv_kernel_copy
    char *argv_kernel_copy[argc];
    copy_argv_pointers_from_user_task(current_task, argv_usr_addr, argv_kernel_copy, argc);

    // Copy the strings in user space to kernel space
    // argv is now the equivalent of the user space argv
    char **argv = (char **) kmalloc(argc * sizeof(char *));
    for (int i = 0; i < argc; i++) {
        argv[i] = (char *) kmalloc(MAX_CMMD_ARG_LEN * sizeof(char));
    }    
    for (int i = 0; i < argc; i++) {
        copy_string_from_user_task(current_task, argv_kernel_copy[i], argv[i], MAX_CMMD_ARG_LEN);
    }

    struct process *process = 0;
    int rc = process_load_with_args(filename, &process, argc, argv);
    if (rc < 0) {
        return 0;
    }

    task_exec(process->task);
    return 0;
}

/* The argv_usr_addr is a user space address that points to an array of char pointers.
 * Since the kernel and user space may not share the same page tables, we need to make sure that this array of character pointers
 * is mapped into a kernel space address that we can access.
 * 
 * After this function has been called, kernel_argv will contain the (kernel accessible) array of char pointers.
 * These char pointers point to strings which must also be mapped into kernel space. See copy_string_from_user_task.
 * 
 * TODO [RyanStan 11-24-23] It would be easier to convert the user space argv into a physical address,
 *                          that way I don't need to switch into user space to access the array of arg char pointers.
*/
int copy_argv_pointers_from_user_task(struct task *task, char *argv_usr_addr[], char *kernel_argv[], int argc)
{
    if (argc * MAX_CMMD_ARG_LEN >= PAGING_PAGE_SIZE)
        return -EINVARG;

    uint32_t *task_page_directory = task->paging->pgd;

    /* Get the page table entry in task's page tables that corresponds to kernel_argv.
     * Since we will be temporarily remapping this page table entry, we will want to replace it
     * once we're done so that the task does not lose access to the virtual address mapping in case it was being used.
     */
    uint32_t old_entry = paging_get_pte(task_page_directory, kernel_argv);

    /* Map the physical address kernel_argv to virtual address kernel_argv within the task's page tables. */
    paging_map_range(task->paging, kernel_argv, kernel_argv, 1, PAGING_READ_WRITE | PAGING_PRESENT | PAGING_USER_SUPERVISOR);
    
    /* Switch to the task's page tables.
     * Now, we can access the value at task_virt_addr.
     */
    paging_switch(task->paging);

    for (int i = 0; i < argc; i++) {
        kernel_argv[i] = argv_usr_addr[i];
    }

    /* Switch back to kernel page tables */
    swap_kernel_page_tables();

    /* Restore task's page tables so that it can see whatever was at kernel_argv before we swapped out that page table entry */
    if (paging_set(task_page_directory, kernel_argv, old_entry) < 0) {
        return -EIO;
    }

    return 0;
}