FLAGS = -g -ffreestanding -nostdlib -O0 -fpic
PROG = print

all:
	nasm -f elf ./$(PROG).asm -o ./build/$(PROG).o
	i686-elf-gcc $(FLAGS) -T ./linker.ld ./build/$(PROG).o -o ./$(PROG).bin

clean: 
	rm -f ./build/$(PROG).o
	rm -f ./$(PROG).bin