#include "coniferos.h"

int main(int argc, char **argv)
{
    print("Hello from main.c!\n");

    for(;;) {
        if (get_key() != 0) {
            print("Key was pressed\n");
        }
    }
    return 0;
}