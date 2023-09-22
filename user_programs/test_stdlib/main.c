#include "coniferos.h"
#include "stdlib.h"

static void test_malloc() {
    void *ptr = malloc(512);
    // TODO [RyanStan 09/22/23] 
    // Use strcpy and print to test this allocation (I haven't implemented these in the stdlib yet)
    if (ptr) {
        print("Memory was allocated\n");
    }
}

int main(int argc, char **argv)
{
    print("Hello from main.c!\n");
    test_malloc();

    for(;;) {
        if (get_key() != 0) {
            print("Key was pressed\n");
        }
    }
    return 0;
}