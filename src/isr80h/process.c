#include "process.h"
#include "task/task.h"
#include "task/process.h"

void *isr80h_command_6_exec(struct interrupt_frame *frame)
{
    struct task *current_task = get_current_task();
    void *filename_usr_addr = task_get_stack_item(current_task, 0);
    char filename[MAX_FILE_PATH_CHARS];
    copy_string_from_user_task(current_task, filename_usr_addr, filename, MAX_FILE_PATH_CHARS);

    struct process *process = 0;
    int rc = process_load(filename, &process);
    if (rc < 0) {
        return 0;
    }

    task_exec(process->task);
    return 0;
}