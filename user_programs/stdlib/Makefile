FLAGS = -g -ffreestanding -nostdlib -fpic
 
MODULES = build/start.o build/coniferos.asm.o build/coniferos.o build/stdlib.o build/stdio.o build/string.o build/memory.o
LIBRARY = stdlib
INCLUDES = ./src

all: $(MODULES)
#	Create a relocatable ELF file so that this can be used as a static library
	i686-elf-ld -m elf_i386  -relocatable $(MODULES) -o ./$(LIBRARY).elf

build/start.o: ./src/start.asm
	nasm -f elf ./src/start.asm -o ./build/start.o

build/coniferos.asm.o: ./src/coniferos.asm
	nasm -f elf ./src/coniferos.asm -o ./build/coniferos.asm.o

build/coniferos.o: ./src/coniferos.c
	i686-elf-gcc -I ./ -I $(INCLUDES) $(FLAGS) -c $^ -o $@

build/stdlib.o: ./src/stdlib.c
	i686-elf-gcc -I ./ -I $(INCLUDES) $(FLAGS) -c $^ -o $@

build/stdio.o: ./src/stdio.c
	i686-elf-gcc -I ./ -I $(INCLUDES) $(FLAGS) -c $^ -o $@

build/string.o: ./src/string.c
	i686-elf-gcc -I ./ -I $(INCLUDES) $(FLAGS) -c $^ -o $@

build/memory.o: ./src/memory.c
	i686-elf-gcc -I ./ -I $(INCLUDES) $(FLAGS) -c $^ -o $@

clean: 
	rm -f .$(MODULES)
	rm -f ./$(LIBRARY).elf