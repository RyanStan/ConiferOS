FLAGS := -O0 -Wall -g -ffreestanding \
	-nostartfiles -nodefaultlibs \
	-nostdlib -lgcc -fno-exceptions \
	-falign-jumps -falign-functions \
	-falign-labels -falign-loops \
	-fomit-frame-pointer -fno-builtin 

all: bin/disk.img bin/kernel.bin

bin/disk.img: bin/boot.bin
	dd if=bin/boot.bin > bin/disk.img
	dd if=bin/kernel.bin >> bin/disk.img
	dd if=/dev/zero bs=512 count=100 >> bin/disk.img # Fills up rest of disk with 100 sector sized blocks of zeros

bin/boot.bin: src/boot/boot.asm
	nasm -f bin src/boot/boot.asm -o bin/boot.bin

bin/kernel.bin: src/kernel.c
	i686-elf-gcc $(FLAGS) -T src/linker.ld src/kernel.c -o bin/kernel.bin

run:
	qemu-system-i386 -drive file=bin/disk.img,index=0,media=disk,format=raw

clean:
	rm -f bin/boot.bin
	rm -f bin/kernel.bin
	rm -f bin/disk.img

