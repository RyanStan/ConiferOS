FLAGS = -g -ffreestanding -nostdlib -O0 -fpic
# Files under modules will be built as relocatable ELF files
MODULES = build/start.o build/coniferos.o
LIBRARY = stdlib
INCLUDES = 

all: $(MODULES)
#	Create a relocatable ELF file so that this can be used as a static library
	i686-elf-ld -m elf_i386  -relocatable $(MODULES) -o ./$(LIBRARY).elf

build/start.o: ./src/start.asm
	nasm -f elf ./src/start.asm -o ./build/start.o

build/coniferos.o: ./src/coniferos.asm
	nasm -f elf ./src/coniferos.asm -o ./build/coniferos.o

clean: 
	rm -f .$(MODULES)
	rm -f ./$(LIBRARY).elf