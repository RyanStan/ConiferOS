#include "coniferos.h"
#include "stdlib.h"

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
    char str[12] = {0};
    itoa(12345, str);
    print(str);
    print("\n");
}

int main(int argc, char **argv)
{
    print("Hello from main.c!\n");
    
    test_itoa();
    test_malloc_and_free();

    for(;;) {
        if (get_key() != 0) {
            print("Key was pressed\n");
        }
    }
    return 0;
}