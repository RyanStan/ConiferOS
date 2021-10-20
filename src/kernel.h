#ifndef KERNEL_H
#define KERNEL_H

void kernel_main();

#define ERROR(value) (void*)(value)
#define ERROR_I(value) (int)(value)
#define IS_ERROR(value) ((int)(value) < 0)
#endif /* KERNEL_H */
