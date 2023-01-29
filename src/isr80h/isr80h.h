#ifndef ISR80H_H
#define ISR80H_H

/* This will be used to associate syscalls with an integer (command_id) */
enum SystemCalls
{
    SYSTEM_COMMAND_0_SUM
};

/* Registers all kernel commands that are defined in isr80h/misc */
void isr80h_register_commands();

#endif