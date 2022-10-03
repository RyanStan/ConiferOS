# ConiferOS
This is the OS I'm building as I follow along with
the Udemy course, "Developing a Multithreaded Kernel From Scratch", 
by Daniel McCarthy.

This kernel is built on a QEMU x86 System emulator.  
You can find the emulated hardware specs here: [i440fx PC](https://www.qemu.org/docs/master/system/i386/pc.html).

## How to Run
- `sudo apt upgrade`
- `sudo apt install nasm`
- `sudo apt install qemu-system-x86`
- Download binutils into `$HOME/src`
- Download gcc into `$HOME/src`
- Clone the repo
- `scripts/install_gcc_deps.sh` (one time only)
- `scripts/build_binutils.sh` (one time only)
- `scripts/build_gcc.sh` (one time only)
- `scripts/build.sh`
- `make run` to run the code

At some point, I want to simplify this setup process.  It's somewhat redundant.

## Overview

### Booting
The BIOS will load the boot sector into memory and execute it.
Our FAT (File Allocation Table filesystem) boot sector is defined by [boot.asm](src/boot/boot.asm) and gets written to the first sector in our disk image.

Our bootloader does two main things: enter protected mode and load our kernel (100 sectors of the disk, 1 KB) into memory at 0x0100000 and then jump to it.

### The Kernel Entry Point
Once the kernel is in memory, the first function to execute is
`_start` in [kernel.asm](src/kernel.asm).  From here, I call kernel_main
which is in [kernel.c](src/kernel.c).  

### Interupts
The hardware emulated by QEMU provides a traditional PIC.  
An interface to configure the interrupt descriptor table is provided by 
[idt.h](src/idt/idt.h). In [kernel.c](src/kernel.c), I initialize the idt with `idt_init()`.

### Paging
From [kernel.c](src/kernel.c):
```
struct paging_desc *paging = init_page_tables(PAGING_READ_WRITE | PAGING_PRESENT | PAGING_USER_SUPERVISOR);
paging_switch(get_pgd(paging));
enable_paging();
```

An interface to interact with the page global directory and to enable paging is provided by 
[paging.h](src/memory/paging/paging.h).  Currently, paging is configured so that every linear (and virtual) address 
has a 1:1 relationship with the corresponding physical address. E.g. the linear address 0x20 maps to physical address 0x20.

### The Heap
An interface to generate heaps, which manage a pool of memory (technically virtual address space),
is defined in [heap.h](src/memory/heap/heap.h) and the allocation algorithm is described in 
the [heap README](src/memory/heap/README.md).  

Since I want all kernel processes to use the same heap, [kernel_heap.h](src/memory/heap/kernel_heap.h)
provides the kernel code with a heap to use for allocating memory.

### I/O
[io.h](src/io/io.h) provides an interface to interact with external hardware via the 
emulated system's I/O bus.  This is crucial for interacting with external hardware like
the hard disk drive.

### Disk Operations
[disk.h](src/disk/disk.h) provides structures to represent attached disks and to 
read blocks from a given disk.  The entry point to search for disks and generate the needed structures is `disk_search_and_init()`.
However, dynamic disk mounting isn't available yet,
and therefore the code expects to have only one disk formatted for the FAT filesystem attached.

To make life easier, [disk_stream.h](src/disk/disk_stream.h) allows us to read and write arbitrary sizes from disks 
by providing a disk stream interface.  This is the foundation for filesystem drivers.

### The Virtual Filesystem Layer
Much like how Linux provides a virtual file system independent of the underlying 
filesystem implementation, [file.h](src/fs/file.h), allows for filesystems to be dynamically inserted at runtime
(see `fs_insert_filesystem`).  These dynamically inserted filesystems must conform to the 
`filesystem` struct.  This means they must provide several functions like `fs_open`, `resolve`, `fs_read`, etc.
The `resolve` function is especially important: when initializing disks, the disk code will attempt to resolve each
disk against a given filesystem.  Thus, each filesystem must implement this `resolve` function so that the disk layer can pair 
a filesystem implementation to a given disk.

### FAT Filesystem
The FAT filesystem is the only filesystem I've implemented so far.  See [fat16.h](src/fs/fat/fat16.h).

## What's Next?
I'm still working through the course.  Topics left are implementing processes, creating user-space
functionality, making paging useful, creating an ELF loader, kernel commands, and libraries like stdlib.

Once I finish the course, there are several things I want to experiment with:
- A networking stack.  The emulator provides a network card to interact with.
- A dynamic device driver layer so that the kernel can support interacting with different hardware by loading in different drivers at runtime.
- Making the heap allocation algorithm more efficient.  Right now, it's a fragmented mess. 
- I'd like to implement an ext filesystem.
- It would be neat to run the OS on bare-metal. I'd have to find an old machine and disk drive.