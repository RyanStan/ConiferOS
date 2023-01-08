FLAGS = -g -ffreestanding -nostdlib -O0 -fpic

all:
	nasm -f elf ./blank.asm -o ./build/blank.o
	i686-elf-gcc $(FLAGS) -T ./linker.ld ./build/blank.o -o ./blank.bin

clean: 
	rm -f ./build/blank.o