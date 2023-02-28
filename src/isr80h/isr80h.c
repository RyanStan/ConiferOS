#include "isr80h/isr80h.h"
#include "idt/idt.h"
#include "isr80h/misc.h"
#include "isr80h/io.h"

void isr80h_register_commands()
{
    isr80h_register_command(SYSTEM_COMMAND_0_SUM, isr80h_command_0_sum);
    isr80h_register_command(SYSTEM_COMMAND_1_PRINT, isr80h_command_1_print);
}