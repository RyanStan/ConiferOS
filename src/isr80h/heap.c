#include "heap.h"
#include "task/task.h"
#include "task/process.h"
#include <stddef.h>

void *isr80h_command_4_malloc(struct interrupt_frame *frame)
{
    int size = (int)task_get_stack_item(get_current_task(), 0);
    return process_malloc_syscall_handler(get_current_task()->process, size);
}

void *isr80h_command_5_free(struct interrupt_frame *frame)
{
    void *ptr = task_get_stack_item(get_current_task(), 0);
    process_free_syscall_handler(get_current_task()->process, ptr);
    return 0;
}
