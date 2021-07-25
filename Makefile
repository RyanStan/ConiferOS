FLAGS := -O0 -Wall -g -ffreestanding \
	-nostartfiles -nodefaultlibs \
	-nostdlib -lgcc -fno-exceptions \
	-falign-jumps -falign-functions \
	-falign-labels -falign-loops \
	-fomit-frame-pointer -fno-builtin 

all: bin/disk.img bin/kernel

bin/disk.img: bin/boot.bin
	cp bin/boot.bin bin/disk.img

bin/boot.bin: src/boot/boot.asm
	nasm -f bin src/boot/boot.asm -o bin/boot.bin

bin/kernel: src/kernel.c
	i686-elf-gcc $(FLAGS) -T src/linker.ld src/kernel.c -o bin/kernel

run:
	qemu-system-i386 -drive file=bin/disk.img,index=0,media=disk,format=raw

clean:
	rm -f bin/boot.bin
	rm -f bin/kernel
	rm -f bin/disk.img

