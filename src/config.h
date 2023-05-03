#ifndef CONFIG_H
#define CONFIG_H

/* selector = byte index into GDT for corresponding segment descriptor */
#define KERNEL_CODE_SELECTOR         0x08;
#define KERNEL_DATA_SELECTOR         0x10;

/* TODO: in an ideal system, this wouldn't be statically defined */
#define KERNEL_HEAP_SIZE 	        104857600	                            /* 100 MB */
#define HEAP_BLOCK_SIZE		        4096

/* Refer to OSDev Wiki Memory Map article */
#define KERNEL_HEAP_ADDRESS	        0x01000000	
#define KERNEL_HEAP_TABLE_ADDR	    0x00007E00	                            /* Ok to use as long as it's < 480.5 KiB */

#define MAX_FILE_PATH_CHARS         128

#define MAX_FILESYSTEMS             12                                      /* The max # of filesystems drivers which can be loaded into the kernel (only at compile time for now)*/
#define MAX_OPEN_FILES              512                                     /* Max # of open file descriptors at once */

#define TOTAL_GDT_SEGMENTS          6                                       /* The number of segments described by the GDT */

#define KERNEL_STACK_ADDR           0x600000                                /* Address of the kernel stack. Loaded into esp on switch to kernel mode. */

/* TODO: the stack should grow downward from high addr to low.  Below the stack (i.e. at lower address should be ip, not higher). */
#define TASK_LOAD_VIRTUAL_ADDRESS   0x400000                                /* An arbitrary virtual memory address to load task executable code into (4 MB) */

#define TASK_STACK_SIZE             1024 * 16
#define TASK_STACK_VIRT_ADDR        0x3FF000                                /* Default stack pointer address for new process.  4096 B below ip. */
#define TASK_STACK_VIRT_ADDR_END    TASK_STACK_VIRT_ADDR - TASK_STACK_SIZE  /* Stack grows downards, so the end address is less than the start*/

/*
 * https://wiki.osdev.org/Segment_Selector - structure of segment registers
 * 0x1b = 0001 1011 - RPL = 3, TI = 0 (use gdt), index = 011000 = 24 bytes = 4rd entry
 * 0x23 = 0010 0011 - RPL = 3, TI = 0 (use gdt), index = 100000 = 32 bytes = 5th entry
 */
#define USER_CODE_SEGMENT           0x1b
#define USER_DATA_SEGMENT           0x23
#define KERNEL_DATA_SEGMENT         0x10

#define PROCESS_MAX_ALLOCATIONS     1024                                    /* Max # of memory allocations that a process can make */

#define MAX_PROCESSES               12

#define MAX_ISR80H_COMMANDS         1024

#define KEYBOARD_BUFFER_SIZE        1024

#endif
