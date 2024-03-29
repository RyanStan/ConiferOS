#include "keyboard.h"
#include "io/io.h"
#include <stdint.h>
#include "ps2_keyboard.h"
#include <stddef.h>
#include "print/print.h"
#include "status.h"

/* PS/2 keyboard driver. 
 * This driver can handle standard PS/2 keyboards using Raw Scan Code Set 1.
 * The Linux Kernel version of this driver can be found in 'linux/drivers/input/keyboard/atkbd.c'. 
 * 
 * We'll be interacting with the Intel 8042 chip, this is the PS/2 controller. https://wiki.osdev.org/%228042%22_PS/2_Controller.
 * See the README (PS/2 Keyboard) for more details on the emulated hardware and buses involved.
 */

#define PS2_PORT                        0x64
#define PS2_COMMAND_ENABLE_FIRST_PORT   0xAE
#define KEY_RELEASED_BIT                0x80

/* From OSDev: "The Data Port (IO Port 0x60) is used for reading data 
 * that was received from a PS/2 device or from the PS/2 controller itself 
 * and writing data to a PS/2 device or to the PS/2 controller itself."
 */
#define PS2_DATA_PORT 0x60

/* Interrupt handler for PS/2 Controller.
 * Reads the scan code from the data port and pushes the 
 * appropriate ascii character to the current process's keyboard buffer.
 */
void ps2_keyboard_handle_interrupt();

/* Initializes the keyboard.
 * Registers the interrupt handler and enables the PS/2 port.
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
    outb(PS2_PORT, PS2_COMMAND_ENABLE_FIRST_PORT);
}

int ps2_keyboard_init()
{
    idt_register_interrupt_handler(KEYBOARD_INTERRUPT_NO, ps2_keyboard_handle_interrupt);
    enable_ps2_first_port();
    return 0; 
}


struct keyboard *get_ps2_keyboard_driver()
{
    return &ps2_keyboard;
}

/* Convert a scan code from scan set one to the ascii value of the key press that
 * it represents. Returns the ascii value.
 * E.g. scan code 0x02 = 1 pressed = ascii for '1' (decimal 49) is returned.
 * Returns < 0 on failure.
 */
uint8_t ps2_keyboard_scancode_to_char(uint8_t scancode)
{
    size_t num_elements_scan_set = sizeof(keyboard_scan_set_one) / sizeof(uint8_t);
    if (scancode > num_elements_scan_set) {
        print("ps2_keyboard.c scancode_to_char: invalid argument");
        return -EINVARG;
    }

    char c = keyboard_scan_set_one[scancode];
    // TODO: check if shift key is pressed to know whether to use capital or lower case

    return c;
}

/* Interrupt handler for PS/2 Controller.
 * Reads the scan code from the data port and pushes the 
 * appropriate ascii character to the current process's keyboard buffer.
 */
void ps2_keyboard_handle_interrupt()
{
    uint8_t scancode = insb(PS2_DATA_PORT);
    insb(PS2_DATA_PORT);                            // ignore byte sent after the scan code. TODO: is this necessary?

    if (scancode & KEY_RELEASED_BIT) {
        // We don't want to handle any key release events
        return;
    }

    uint8_t c = ps2_keyboard_scancode_to_char(scancode);
    if (c > 0) {
        // Push the character to the current process's keyboard buffer.
        keyboard_push(c);
    }
}
