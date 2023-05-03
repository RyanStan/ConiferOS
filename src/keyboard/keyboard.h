#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "config.h"
#include "task/process.h"

// Virtual keyboard layer so that we can support different physical keyboards

typedef int (*keyboard_init_function)();

struct keyboard
{
    keyboard_init_function init;
    char name[20];
    struct keyboard *next;
};

// Remove the last character from the process's keyboard buffer.
void keyboard_backspace(struct process *process);

// Pushes a character to the current process's keyboard buffer.
void keyboard_push(char c);

// Pop a character from the current task's process's keyboard buffer
char keyboard_pop();

// In the future, this will initialize keyboard drivers.
void keyboard_init();

#endif

