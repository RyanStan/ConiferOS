#include "coniferos.h"
#include "stdlib.h"
#include "stdio.h"

static void test_malloc_and_free() {
    void *ptr = malloc(512);
    // TODO [RyanStan 09/22/23] 
    // Use strcpy and print to test this allocation (I haven't implemented these in the stdlib yet)
    if (ptr) {
        print("Memory was allocated\n");
    }
    free(ptr);

    // Validate that this throws a page fault
    //char *char_ptr = (char *)ptr;
    //char_ptr[0] = 'A';
}

static void test_itoa()
{
    print("Testing itoa.........\n");
    char str[12] = {0};
    itoa(12345, str);
    print(str);
    print("\n");
}

static void test_putchar()
{
    print("Testing putchar.........\n");
    putchar('Z');
    print('\n');
}

static void test_printf()
{
    printf("This is a %s test. Here is a random integer: %i\n", "printf", 10);
}

int main(int argc, char **argv)
{
    print("Hello from main.c!\n");
    char buf[1024];
    coniferos_terminal_readline(buf, 1024);
    
    test_putchar();
    test_itoa();
    test_malloc_and_free();
    test_printf();

    for(;;) {
        if (coniferos_get_key() != 0) {
            print("Key was pressed\n");
        }
    }
    return 0;
}