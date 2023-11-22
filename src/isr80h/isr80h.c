#include "isr80h/isr80h.h"
#include "idt/idt.h"
#include "isr80h/misc.h"
#include "isr80h/io.h"
#include "isr80h/heap.h"
#include "isr80h/process.h"

void isr80h_register_commands()
{
    isr80h_register_command(SYSTEM_COMMAND_0_SUM, isr80h_command_0_sum);
    isr80h_register_command(SYSTEM_COMMAND_1_PRINT, isr80h_command_1_print);
    isr80h_register_command(SYSTEM_COMMAND_2_GET_KEY_PRESS, isr80h_command_2_get_key_press);
    isr80h_register_command(SYSTEM_COMMAND_3_PUT_CHAR_ON_DISPLAY, isr80h_command_3_put_char_on_display);
    isr80h_register_command(SYSTEM_COMMAND_4_MALLOC, isr80h_command_4_malloc);
    isr80h_register_command(SYSTEM_COMMAND_5_FREE, isr80h_command_5_free);
    isr80h_register_command(SYSTEM_COMMAND_6_EXECVE, isr80h_command_6_execve);
}