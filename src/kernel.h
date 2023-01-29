#ifndef KERNEL_H
#define KERNEL_H

/* These macros are useful for dealing with functions that return void pointers */
#define ERROR(value) (void*)(value)
#define ERROR_I(value) (int)(value)
#define IS_ERROR(value) ((int)(value) < 0)

void kernel_main();

/* Prints msg to the console and then enters an infinite loop.
 * Should be used as a safety measure upon detecting an internal fatal
 * error in which the OS is unable to safely recover or continuing
 * to run the system would have a higher risk of major data loss.
 */
void panic(const char *msg);

/* Swaps out current task's page tables and swaps in the kernel page tables 
 * It also sets ds, es, fs, gs segment registers to the kernel data segment.
 */
void swap_kernel_page_tables();

#endif /* KERNEL_H */
