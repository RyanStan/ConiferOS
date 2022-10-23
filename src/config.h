#ifndef CONFIG_H
#define CONFIG_H

#define KERNEL_CODE_SELECTOR         0x08;
#define KERNEL_DATA_SELECTOR         0x10;

/* TODO: in an ideal system, this wouldn't be statically defined */
#define KERNEL_HEAP_SIZE 	        104857600	    /* 100 MB */
#define HEAP_BLOCK_SIZE		        4096

/* Refer to OSDev Wiki Memory Map article */
#define KERNEL_HEAP_ADDRESS	        0x01000000	
#define KERNEL_HEAP_TABLE_ADDR	    0x00007E00	    /* Ok to use as long as it's < 480.5 KiB */

#define MAX_FILE_PATH_CHARS         128

#define MAX_FILESYSTEMS             12              /* The max # of filesystems drivers which can be loaded into the kernel (only at compile time for now)*/
#define MAX_OPEN_FILES              512             /* Max # of open file descriptors at once */

#define TOTAL_GDT_SEGMENTS          3               /* The number of segments described by the GDT */

#endif
