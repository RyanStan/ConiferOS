FLAGS = -g -ffreestanding -nostdlib -O0
PROG = tstlib
MODULES = ./build/$(PROG).o
STDLIB = ../stdlib/stdlib.elf
INCLUDES = ../stdlib/src

all: $(MODULES)
# Generates executable (ET_EXEC) ELF file
	i686-elf-gcc $(FLAGS) -T ./linker.ld $(MODULES) $(STDLIB) -o ./$(PROG).elf

build/$(PROG).o: ./$(PROG).c
	i686-elf-gcc -I ./ -I $(INCLUDES) $(FLAGS) -c $^ -o $@

clean: 
	rm -f $(MODULES)
	rm -f ./$(PROG).elf