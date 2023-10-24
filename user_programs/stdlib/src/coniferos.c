#include "coniferos.h"

int coniferos_get_key_blocking()
{
    int key = 0;
    while (key == 0) {
        key = coniferos_get_key();
    }

    return key;
}

int coniferos_terminal_readline(char *out, int max)
{
    int i;

    for (i = 0; i < max; i++) {
        char key = coniferos_get_key_blocking();

        // Carriage return
        if (key == 13) {
            break;
        }

        coniferos_putchar(key);

        // Backspace
        if (key == 0x08 && i >= 1) {
            out[i - 1] = 0x00;
            i -= 2;
            continue;
        }

        out[i] = key;
    }
    out[i] = 0x00;
}