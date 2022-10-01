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
Our FAT (File Allocation Table filesystem) boot sector is defined by [boot.asm](src/boot/boot.asm) which gets written to the first sector in our disk image.

Our bootloader does two main things: enter protected mode and load our kernel (100 sectors of the disk, 1 KB) into memory at 0x0100000 and then jump to it.

### The Kernel Entry Point
The code that the bootloader jumps to once the kernel is in memory 
starts with [kernel.asm](src/kernel.asm).  From here, we call kernel_main
which is in [kernel.c](src/kernel.c).  
