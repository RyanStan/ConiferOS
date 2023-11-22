#include "coniferos.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "memory.h"

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

static void test_strtok()
{
    print("Testing strtok\n");
    const char str[] = "ConiferOS C standard library";

    char token_buf[1024];
    const char *token = strtok(str, ' ', token_buf);
    while (token != NULL) {
        printf("Token: %s\n", token);
        memset(token_buf, 0, sizeof(token_buf));
        token = strtok(NULL, ' ', token_buf);
    }
    print("End of strtok test\n");
}

void test_exec_with_args() 
{
    char path_buf[1024];
    strcpy(path_buf, "0:/echo.elf");
    char *args[] = {
        "first_argument",
        NULL
    };
    coniferos_execve(path_buf, args, 1);
}

int main(int argc, char **argv)
{
    print("Hello from the standard library test!\n");
    /*
    test_putchar();
    test_itoa();
    test_malloc_and_free();
    test_printf();
    test_strtok();
    */
    test_exec_with_args();

    for(;;) {
        if (coniferos_get_key() != 0) {
            print("Key was pressed\n");
        }
    }
    return 0;
}