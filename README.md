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
The hardware emulated by QEMU provides two i8259 PIC devices. The following is a snippet of output from `info qom-tree`
of the QEMU monitor:
```
    /device[4] (isa-i8259)
      /elcr[0] (memory-region)
      /pic[0] (memory-region)
      /unnamed-gpio-in[0] (irq)
      /unnamed-gpio-in[1] (irq)
      /unnamed-gpio-in[2] (irq)
      /unnamed-gpio-in[3] (irq)
      /unnamed-gpio-in[4] (irq)
      /unnamed-gpio-in[5] (irq)
      /unnamed-gpio-in[6] (irq)
      /unnamed-gpio-in[7] (irq)
    /device[5] (isa-i8259)
      /elcr[0] (memory-region)
      /pic[0] (memory-region)
      /unnamed-gpio-in[0] (irq)
      /unnamed-gpio-in[1] (irq)
      /unnamed-gpio-in[2] (irq)
      /unnamed-gpio-in[3] (irq)
      /unnamed-gpio-in[4] (irq)
      /unnamed-gpio-in[5] (irq)
      /unnamed-gpio-in[6] (irq)
      /unnamed-gpio-in[7] (irq)

```
An interface to configure the interrupt descriptor table is provided by 
[idt.h](src/idt/idt.h). In [kernel.c](src/kernel.c), I initialize the idt and load the idtr with `idt_init()`.

Interrupt handlers can be dynamically registered with the `idt_register_interrupt_handler` function in [idt.h](src/idt/idt.h). 
The `interrupt_handler` function is responsible for calling the registered interrupt handler for the raised interrupt.

In [kernel.asm](src/kernel.asm), I've reinitialized the 8259 master PIC to map IRQs to IDT entries starting at 0x20. This is because the processor has already reserved earlier IDT entries for CPU exceptions.  For example, now, if the 8259 has IRQ 0 raised, it will call the entry at index 0x20 in our IDT.

### Paging
From [kernel.c](src/kernel.c):
```
struct paging_desc *kernel_pages = 0;
kernel_pages = init_page_tables(PAGING_READ_WRITE | PAGING_PRESENT | PAGING_USER_SUPERVISOR);
paging_switch(kernel_pages);
enable_paging();
```

An interface to interact with the paging data structures and to enable paging is provided by 
[paging.h](src/memory/paging/paging.h).  Currently, paging for kernel land is configured so that every linear (and virtual) address 
has a 1:1 relationship with the corresponding physical address. E.g. the linear address 0x20 maps to physical address 0x20.
There's no concept of "high memory" for the kernel at the moment, and no split in the virtual address space between user land and kernel land.

In 32-bit x86 with 4-KByte pages, a two level paging scheme is used.  Each linear address maps an index into the page directory,
and index into the corresponding page table, and the byte offset within the page.

I'll talk about paging for user processes under "Processes and Tasks".

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
One very important thing to keep in mind with FAT16: Filename lengths have a strict limit: 8 characters for everything
before the `.`, and 3 characters for the extension.  Forgetting this fact has caused me lots of pain numerous times.

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
Binary executables and ELF executables are supported.  When the kernel initializes a process, it will allocate space for the executable and the stack,
and will map that memory into the page tables of the task that the process encapsulates. For example, see `process_map_task_memory` in [process.c](src/task/process.c).

Eventually, I want to move away from the process abstraction.  The Linux Kernel does not differentiate between processes and threads,
or processes and tasks, and I do not want to either.

### Userland Functionality
The function `process_load` and `task_exec` is used to move the CPU into user mode.

To enter user mode, a process must be loaded with `process_load`.  This will allocate memory for the executable and stack (see `process_load_data`).
`process_load` will also create and initialize a `task` structure.  In `task_init` ([task.h](src/task/task.h)), 
we set the hardware context as follows:

```
task->registers.ip = TASK_LOAD_VIRTUAL_ADDRESS;
if (process->format == ELF) {
    struct elf32_ehdr *elf32_ehdr = elf_get_ehdr(process->elf_file);
    task->registers.ip = elf32_ehdr->e_entry;
}
task->registers.ss = USER_DATA_SEGMENT;
task->registers.cs = USER_CODE_SEGMENT;
task->registers.esp = TASK_STACK_VIRT_ADDR;
```

Then, after `process_load` has initialized the task instance, it will modify the page tables of the task instance to 
map these addresses to the memory that the process has allocated. See `process_map_task_memory`.

Once the task is ready to execute, `task_exec` will swap the task's page tables into the processor's current execution state
by modifying the CR3 register, and then will move the task's saved registers' values into the hardware registers, and then
will call `iretd` to drop into userland.  This will set the code segment and data segment selector registers to map the user code and user data
segments.  The values for these segments are defined in [kernel.c](src/kernel.c) (see `/* Set up the GDT */`).

One other interesting thing worth noting.  When we switch to the task's page tables, before we call `iretd`, we can still
access the correct kernel code memory, because in `task_init`, we do a one to one mapping of physical memory to virtual addresses.
However, at that point, we can only read the kernel memory, not write it. I would like to see how the Linux Kernel handles this small
transition gap, from kernel code having swapped the current page tables, to when the user process actually begins executing.

### Userland to Kernel Communication (int 0x80)
The interrupt 0x80 is used to communicate with the kernel from userland.  The userland program will put a command ID into the eax register,
and push arguments onto the stack.  Given the command ID, the kernel will know which operation (system call) to perform.

The ISR that handles 0x80 is `isr80h_wrapper`, which is defined in [idt.asm](src/idt/idt.asm).  The wrapper saves the state of the user program's
general purpose registers, and then calls `isr80h_handler`.  The definition for this function is in [idt.h](src/idt/idt.h).  This function finishes converting
the processor into kernel mode by loading the kernel page tables into memory, and then dispatches the appropriate system call, given the command id that
was passed to the eax register from the userland program.

An interface for kernel code to register systems calls, or kernel commands, is provided by `isr80h_register_command`, which is declared in [idt.h](src/idt/idt.h).  Right now, the only point at which I register system calls is `isr80h_register_commands` in [isr80h.h](src/isr80h/isr80h.h).

Misc. kernel commands are stored in [`src/isr80h/misc.h`](./src/isr80h/io.h), and IO related kernel commands are stored in [`src/isr80h/io.h`](./src/isr80h/io.h).
Memory related system call are declared in [`src/isr80h/heap.h`](./src/isr80h/heap.h).

### User Programs and the Conifer OS C Standard Library
User programs are stored in the [user_programs](./user_programs/) folder. The example programs here test the stdlib and various ConiferOS system calls.

The [stdlib](./user_programs/stdlib/) folder contains our C standard library, `stdlib.elf`. 

The stdlib contains useful functions for managing heap allocated memory, getting keyboard input, printing, etc.
It also contains an entry point to C user programs, `start`, which which defines the `_start` symbol and is responsible for calling the `main` function.

See [user_programs/test_stdlib](./user_programs/test_stdlib/) for a program which uses the stdlib.

### User space program execution
User programs can launch other programs and pass them arguments by using the ConiferOS exec system calls.
As an example, see the following code from [user_programs/test_stdlib/test_stdlib.c](./user_programs/test_stdlib/tstlib.c):
```
    char path_buf[1024];
    strcpy(path_buf, "0:/echo.elf");
    char *args[] = {
        "first_argument",
        NULL
    };
    coniferos_execve(path_buf, args, /*argc*/1);
```
I implemented the "echo" user space program to showcase command line arguments.


### Virtual Keyboard Layer
Each process structure has a `keyboard_buffer`, which is defined in [process.h](src/task/process.h). An interface for interacting with a process's keyboard
buffer is defined in [keyboard.h](src/keyboard/keyboard.h). The `keyboard_push` function will push a character to the keyboard buffer of the current process,
while `keyboard_pop` pops a character from the current task's process's keyboard buffer.

A userland program can invoke the system call `SYSTEM_COMMAND_2_GET_KEY_PRESS` via interrupt 0x80, to pop a character off the current process's keyboard buffer.
The handler for this system call is defined in [isr80h/io.h](src/isr80h/io.h).

### PS/2 Keyboard
Qemu emulates an Intel 8042 chip, which is the PS/2 Controller. For more info on this chip, see ["'8042' PS/2 Controller" on OSDev](https://wiki.osdev.org/%228042%22_PS/2_Controller). This is the chip that controls communication between the PS/2 Keyboard and the CPU.
If you open the qemu monitor and run `info qtree`, you'll notice that attached to the PCI bus is an ISA bridge
which supports legacy ISA devices by translating PCI I/O and PCI Memory space accesses into ISA I/O and ISA Memory accesses. Then, attached to the ISA bus,
is the i8042 device, which is our Intel 8042 chip. 
```
dev: PIIX3, id ""
        addr = 01.0
        romfile = ""
        romsize = 4294967295 (0xffffffff)
        rombar = 1 (0x1)
        multifunction = true
        x-pcie-lnksta-dllla = true
        x-pcie-extcap-init = true
        failover_pair_id = ""
        acpi-index = 0 (0x0)
        class ISA bridge, addr 00:01.0, pci id 8086:7000 (sub 1af4:1100)
        bus: isa.0
          type ISA
          dev: port92, id ""
            gpio-out "a20" 1
          dev: vmmouse, id ""
          dev: vmport, id ""
            x-read-set-eax = true
            x-signal-unsupported-cmd = true
            x-report-vmx-type = true
            x-cmds-v2 = true
            vmware-vmx-version = 6 (0x6)
            vmware-vmx-type = 2 (0x2)
          dev: i8042, id ""
            gpio-out "a20" 1
            extended-state = true
            kbd-throttle = false
            isa irqs 1,12

```

The PS/2 driver is in the works. IRQ 1 is typically raised by a keyboard device. We've mapped IRQ 1 to IDT entry 0x21.

### ELF (Executable and Linking Format)
As mentioned in "Processes and Tasks", ConiferOS contains an ELF loader which can be used to instantiate user processes
from ELF executables. The loadable segments of the ELF file will be mapped into the address space of the user processes.

The ELF loader code can be found in [src/loader/formats](src/loader/formats/).

Currently, the ELF loader only supports executable files (`e_type` = `ET_EXEC`). Thus, the ELF loader
does not support relocatable files or dynamic shared libraries.

ELF Specification that was used as reference: https://refspecs.linuxfoundation.org/elf/elf.pdf

### Build
TODO: add section on linker script

## What's Next?
There are several things I want to experiment with:
- A networking stack.  The emulator provides a network card to interact with.
- A dynamic device driver layer so that the kernel can support interacting with different hardware by loading in different drivers at runtime.
- Making the heap allocation algorithm more efficient.  Right now, it's a fragmented mess. 
- I'd like to implement an ext filesystem.
- It would be neat to run the OS on bare-metal. I'd have to find an old machine and disk drive.
- The Kernel land code currently disables interrupts whenever it executes, because there aren't any
  safety mechanisms in place to support interleaving.  I would like to add some support for concurrency controls
  for kernel land code.
- A PCI driver so I can dynamically find attached devices. Then I don't have to guess I/O port addresses.
- `mmap` function to map files into memory.