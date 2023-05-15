#include "keyboard.h"
#include "io/io.h"
#include <stdint.h>

/* PS/2 keyboard driver. 
 * This driver can handle standard PS/2 keyboards using Raw Scan Code Set 1.
 * The Linux Kernel version of this driver can be found in 'linux/drivers/input/keyboard/atkbd.c'. 
 * 
 * We'll be interacting with the Intel 8042 chip, this is the PS/2 controller. https://wiki.osdev.org/%228042%22_PS/2_Controller.
 * See the README (PS/2 Keyboard) for more details on the emulated hardware and buses involved.
 */

int ps2_keyboard_init();

/* Scan code set 1. Copied from https://github.com/nibblebits/PeachOS/.
 * You can find the scan code set at https://wiki.osdev.org/PS/2_Keyboard.
 * The index is the scan code, and the value is the ascii value associated with the key that was pressed.
 * For example, index 2 = scan code 0x02 = 1 pressed = ascii for '1' (decimal 49).
 * 
 * We ignored some of the scan codes by setting the value to 0x00.
 */
static uint8_t keyboard_scan_set_one[] = {
    0x00, 0x1B, '1', '2', '3', '4', '5',
    '6', '7', '8', '9', '0', '-', '=',
    0x08, '\t', 'Q', 'W', 'E', 'R', 'T',
    'Y', 'U', 'I', 'O', 'P', '[', ']',
    0x0d, 0x00, 'A', 'S', 'D', 'F', 'G',
    'H', 'J', 'K', 'L', ';', '\'', '`', 
    0x00, '\\', 'Z', 'X', 'C', 'V', 'B',
    'N', 'M', ',', '.', '/', 0x00, '*',
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, '7', '8', '9', '-', '4', '5',
    '6', '+', '1', '2', '3', '0', '.'
};

struct keyboard ps2_keyboard = {
    .init = ps2_keyboard_init,
    .name = {"PS/2"},
    .next = 0
};

void enable_ps2_first_port()
{
    // 0x64 is the command register.
    // 0xAE is the command to enable the first PS/2 port.
    outb(0x64, 0xAE);
}

int ps2_keyboard_init()
{
    enable_ps2_first_port();
    return 0; 
}

struct keyboard *ps2_init()
{
    return &ps2_keyboard;
}

