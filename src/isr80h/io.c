#include "isr80h/io.h"
#include "task/task.h"
#include "print/print.h"
#include "keyboard/keyboard.h"

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

void *isr80h_command_2_get_key_press(struct interrupt_frame *frame)
{
    char c = keyboard_pop();
    return (void *)((int)c);        // Cast to integer and then cast to void*
}

void *isr80h_command_3_put_char_on_display(struct interrupt_frame *frame) 
{
    // Retrieve the character that the user process pushed to the stack as an argument
    char c = (char)(int)task_get_stack_item(get_current_task(), 0);
    terminal_write_char(c, 15);
    return 0;
}