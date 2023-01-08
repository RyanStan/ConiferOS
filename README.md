# ConiferOS
This is the OS I'm building as I follow along with
the Udemy course, "Developing a Multithreaded Kernel From Scratch", 
by Daniel McCarthy.

This kernel is built on a QEMU x86 System emulator.  
You can find the emulated hardware specs here: [i440fx PC](https://www.qemu.org/docs/master/system/i386/pc.html).

## How to Run
### System Setup
- `sudo apt upgrade`
- `sudo apt install nasm`
- `sudo apt install qemu-system-x86`
- Download binutils into `$HOME/src`
- Download gcc into `$HOME/src`
- Clone the repo
- `scripts/install_gcc_deps.sh`
- `scripts/build_binutils.sh`
- `scripts/build_gcc.sh`
### Building and Running
- `scripts/build.sh`
- `make run`

At some point, I want to simplify this process.  It's somewhat redundant.

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
Currently, there's an interrupt handler for int 0x00 and int 0x21 (keyboard interrupt).
These handlers are set in `idt_init` in [idt.c](src/idt/idt.c). 

### Paging
From [kernel.c](src/kernel.c):
```
struct paging_desc *paging = init_page_tables(PAGING_READ_WRITE | PAGING_PRESENT | PAGING_USER_SUPERVISOR);
paging_switch(get_pgd(paging));
enable_paging();
```

An interface to interact with the paging data structures and to enable paging is provided by 
[paging.h](src/memory/paging/paging.h).  Currently, paging for kernel land is configured so that every linear (and virtual) address 
has a 1:1 relationship with the corresponding physical address. E.g. the linear address 0x20 maps to physical address 0x20.
There's no concept of "high memory" for the kernel at the moment, and no split in the virtual address space between user land and kernel land.

In 32-bit x86 with 4-KByte pages, a two level paging scheme is used.  Each linear address maps an index into the page directory,
and index into the corresponding page table, and the byte offset within the page.

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

### GDT
The GDT is loaded into memory and initially configured with Code and Data segments in [boot.asm](src/boot/boot.asm).
However, since the GDT must be modified again down the road, and interface for interacting with the GDT is provided in 
[gdt.h](src/gdt/gdt.h).  Much like the Linux Kernel, ConiferOS doesn't make explicity use of segmentation, 
and instead has kernel/user code and data segments share the entire address space.

### Processes and Tasks
A task is the CPU schedulable unit in ConiferOS.  The task structure is defined in [task.h](src/task/task.h).
Each task has it's own set of page tables.  All runnable tasks are stored together in a linked list structure.
A task structure also maintains its hardware context - registers.  When we begin execution of a task, we load
the values from registers structure into the hardware registers before calling iretd to drop into userland.

A task is owned by a process.  For now, a process only owns one task, but the intent was that a process
can eventually own multiple process.  The process structure is defined in [process.h](src/task/process.h).

When a user attempts to load an executable with `process_load`, a process will be created which contains reference to that executable (`filename`).
For now, only binary executables are supported.  A process, upon initialization, will allocate space for the executable and the stack,
and will map that memory into the page tables of the task that the process encapsulates.

Eventually, I want to move away from the process abstraction.  The Linux Kernel does not differentiate between processes and threads,
or processes and tasks, and I do not want to either.

### Userland Functionality
Currently, you can drop the processor into userland via `process_load` and then a subsequent `task_exec`.  However, there's no
support for interacting with the kernel from userland and exiting back into kernel code.

To drop into userland, you must first load a process with `process_load`.  This will allocate memory for the executable and stack (see `process_load_data`).
`process_load` will also create and initialize a `task` structure.  In `task_init` ([task.h](src/task/task.h)), 
we set the hardware context as follows:

```
task->registers.ip = TASK_LOAD_VIRTUAL_ADDRESS;
task->registers.ss = USER_DATA_SEGMENT;
task->registers.cs = USER_CODE_SEGMENT;
task->registers.esp = TASK_STACK_VIRT_ADDR;
```

Then, after `process_load` has initialized the task instance, it will modify the page tables of the task instance to 
map these addresses to the memory that the process has allocated. See `process_map_task_memory`.

Once the task is ready to execute, `task_exec` will swap the task's page tables into the processor's current execution state
by modifying the CR3 register, and then will move the task's saved registers' values into the hardware registers, and then
will call iretd to drop into userland.  This will set the code segment and data segment selector registers to map the user code and user data
segments.  The values for these segments are defined in [kernel.c](src/kernel.c) (see `/* Set up the GDT */`).

One other interesting thing worth noting.  When we switch to the task's page tables, before we call `iretd`, we can still
access the correct kernel code memory, because in `task_init`, we do a one to one mapping of physical memory to virtual addresses.
However, at that point, we can only read the kernel memory, not write it. I would like to see how the Linux Kernel handles this small
transition gap, from kernel code having swapped the current page tables, to when the user process actually begins executing.


### Build
TODO: add section on linker script

## What's Next?
I'm still working through the course.  Topics left are implementing processes, creating user-space
functionality, making paging useful, creating an ELF loader, kernel commands, and libraries like stdlib.

Once I finish the course, there are several things I want to experiment with:
- A networking stack.  The emulator provides a network card to interact with.
- A dynamic device driver layer so that the kernel can support interacting with different hardware by loading in different drivers at runtime.
- Making the heap allocation algorithm more efficient.  Right now, it's a fragmented mess. 
- I'd like to implement an ext filesystem.
- It would be neat to run the OS on bare-metal. I'd have to find an old machine and disk drive.
- The Kernel land code currently disables interrupts whenever it executes, because there aren't any
  safety mechanisms in place to support interleaving.  I would like to add some support for concurrency controls
  for kernel land code.