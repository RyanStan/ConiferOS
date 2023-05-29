#ifndef ISR80H_H
#define ISR80H_H

/* This will be used to associate syscalls with an integer (command_id) */
enum SystemCalls
{
    SYSTEM_COMMAND_0_SUM,
    SYSTEM_COMMAND_1_PRINT,
    SYSTEM_COMMAND_2_GET_KEY_PRESS,
    SYSTEM_COMMAND_3_PUT_CHAR_ON_DISPLAY,
};

/* Registers all kernel commands that are defined in isr80h/misc */
void isr80h_register_commands();

#endif