FLAGS = -g -ffreestanding -nostdlib -O0
PROG = main
# Files under modules will be built as relocatable ELF files
MODULES = ./build/$(PROG).o
STDLIB = ../stdlib/stdlib.elf
INCLUDES = ../stdlib/src

all: $(MODULES)
#	Link our relocatable files together into an executable ELF file
	i686-elf-gcc $(FLAGS) -T ./linker.ld $(MODULES) $(STDLIB) -o ./$(PROG).elf

build/$(PROG).o: ./$(PROG).c
	i686-elf-gcc -I ./ -I $(INCLUDES) $(FLAGS) -c $^ -o $@

clean: 
	rm -f $(MODULES)
	rm -f ./$(PROG).elf