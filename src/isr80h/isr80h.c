#include "isr80h/isr80h.h"
#include "idt/idt.h"
#include "isr80h/misc.h"

void isr80h_register_commands()
{
    isr80h_register_command(SYSTEM_COMMAND_0_SUM, isr80h_command_0_sum);
}