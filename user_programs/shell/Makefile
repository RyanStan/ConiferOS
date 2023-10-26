FLAGS = -g -ffreestanding -nostdlib -O0
PROG = shell
MODULES = ./build/$(PROG).o
STDLIB = ../stdlib/stdlib.elf
INCLUDES = ../stdlib/src

all: $(MODULES)
# Generates executable (ET_EXEC) ELF file
	i686-elf-gcc $(FLAGS) -T ./linker.ld $(MODULES) $(STDLIB) -o ./build/$(PROG).elf

build/$(PROG).o: ./src/$(PROG).c
	i686-elf-gcc -I ./ -I $(INCLUDES) $(FLAGS) -c $^ -o $@

clean: 
	rm -f $(MODULES)
	rm -f ./build/$(PROG).elf