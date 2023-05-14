#include "keyboard.h"
#include "status.h"
#include "kernel.h"
#include "task/process.h"
#include "task/task.h"
#include "print/print.h"
#include "keyboard/ps2_keyboard.h"

static struct keyboard *keyboard_list_head = 0;
static struct keyboard *keyboard_list_last = 0;


void keyboard_init()
{
    keyboard_insert(ps2_init());
}

int keyboard_insert(struct keyboard *keyboard)
{
    if (!keyboard->init) {
        return -EINVARG;
    }

    if (keyboard_list_last) {
        keyboard_list_last->next = keyboard;
        keyboard_list_last = keyboard;
    } else {
        // The keyboard list must have been empty.
        keyboard_list_head = keyboard;
        keyboard_list_last = keyboard;
    }

    return keyboard->init();
}

// Increment the tail of the circular buffer. Ensures that it stays within the bounds of the buffer array.
void increment_tail(struct keyboard_buffer *keyboard_buffer)
{
    int next_tail_val = (keyboard_buffer->tail + 1) % sizeof(keyboard_buffer->buffer);
    // Check to see if the buffer is full. If this is true, the next write will overwrite the head.
    if (next_tail_val == keyboard_buffer->head) {
        print("ERROR: Keyboard buffer overflow\n");
    }
    keyboard_buffer->tail = (keyboard_buffer->tail + 1) % sizeof(keyboard_buffer->buffer);
}

void keyboard_push(char c)
{
    struct process *process = get_current_process();
    if (!process)
        return;

    struct keyboard_buffer *keyboard_buffer = &process->keyboard_buffer;
    keyboard_buffer->buffer[keyboard_buffer->tail] = c;
    increment_tail(keyboard_buffer);
}

// Increment the head of the circular buffer. Ensures that it stays within the bounds of the buffer array.
void increment_head(struct keyboard_buffer *keyboard_buffer)
{
    keyboard_buffer->head = (keyboard_buffer->head + 1) % sizeof(keyboard_buffer->buffer);
}

// Decrement the tail. Will ensure tail stays within bounds of buffer. 
// noop if head == tail.
void decrement_tail(struct keyboard_buffer *keyboard_buffer)
{
    if (keyboard_buffer->tail == keyboard_buffer->head) {
        return;
    }
    
    if (keyboard_buffer->tail == 0) {
        keyboard_buffer->tail = sizeof(keyboard_buffer->buffer);
        return;
    }
            
    keyboard_buffer->tail--;
}

void keyboard_backspace(struct process *process)
{
    decrement_tail(&process->keyboard_buffer);
    process->keyboard_buffer.tail = 0x00;
}

char keyboard_pop()
{
    // Pop from the current task's process. 
    // TODO: I'm not sure why push uses the current process but pop uses the current task. I guess I'll find out.

    struct task *task = get_current_task();
    if (!task)
        return 0;
    
    struct keyboard_buffer *keyboard_buffer = &task->process->keyboard_buffer;

    char val = keyboard_buffer->buffer[keyboard_buffer->head];
    if (val == 0x00) {
        return 0;
    }

    keyboard_buffer->buffer[keyboard_buffer->head] = 0;
    increment_head(keyboard_buffer);
    return val;
}