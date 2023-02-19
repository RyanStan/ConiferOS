#include "isr80h/misc.h"
#include "task/task.h"

void *isr80h_command_0_sum(struct interrupt_frame *frame)
{
    struct task *current_task = get_current_task();
    int n1 = (int)task_get_stack_item(current_task, 0);
    int n2 = (int)task_get_stack_item(current_task, 1);
    return (void*) n1 + n2;
}