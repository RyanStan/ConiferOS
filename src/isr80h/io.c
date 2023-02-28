#include "isr80h/io.h"
#include "task/task.h"
#include "print/print.h"

#define MAX_STR_SIZE 1024

void *isr80h_command_1_print(struct interrupt_frame *frame)
{
    struct task *current_task = get_current_task();

    // Get the address of the message to print from task's stack.
    void *user_message = task_get_stack_item(current_task, 0);

    char buf[MAX_STR_SIZE];

    copy_string_from_user_task(current_task, user_message, buf, sizeof(buf));

    print(buf);

    return 0;
}