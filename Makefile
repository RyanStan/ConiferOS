# 	NOTES
#
# 	os.bin is the flat hard disk that the emulator will see
#
# 	our only module right now is kernel.asm but in the future that will change
#
# 	what the bin/kernel.bin target recipe is doing:
# 		-the first command links together all our object files into another large intermediary relocatable object file (build/kernelfull.o)
# 		-the second command tells gcc to use our linker script and ld to turn our intermediary relocatable object file (build/kernelfull.o) into a binary at bin/kernel.bin
# 		  using gcc since in the future we will be compiling c files in with this command.  When an object file is passed to gcc, gcc
# 		  skips compilation and instead directly invokes the linker (i686-elf-ld)
#
#	not entirely sure if we actually need fomit-frame-pointer flag for gcc
#	some of the flags I included are probably redundant and some don't actually show up in the man page
#

SHELL = /bin/sh
MODULES = build/kernel.asm.o build/kernel.o build/print.o build/idt/idt.asm.o build/idt/idt.o build/memory/memory.o build/io/io.asm.o
INCLUDES = ./src
FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc

#TODO: add environment variables from build.sh so I don't need to use that script

all: bin/disk.img

bin/disk.img: bin/os.bin
	cp bin/os.bin bin/disk.img

bin/os.bin: bin/boot.bin bin/kernel.bin
	dd if=bin/boot.bin > bin/os.bin
	dd if=bin/kernel.bin >> bin/os.bin
	dd if=/dev/zero bs=512 count=100 >> bin/os.bin # Fills up rest of disk with 100 sector sized blocks of zeros (we'll be loading our kernel in eventually over these null sectors)

bin/kernel.bin: $(MODULES)
	i686-elf-ld -g -relocatable $(MODULES) -o build/kernelfull.o
	i686-elf-gcc $(FLAGS) -T src/linker.ld -o bin/kernel.bin -ffreestanding -O0 -nostdlib build/kernelfull.o
	
bin/boot.bin: src/boot/boot.asm
	nasm -f bin src/boot/boot.asm -o bin/boot.bin

build/kernel.asm.o:  src/kernel.asm
	nasm -f elf -g src/kernel.asm -o build/kernel.asm.o

build/kernel.o: src/kernel.c
	i686-elf-gcc -I $(INCLUDES) $(FLAGS) -c src/kernel.c -o build/kernel.o

build/print.o: src/print/print.c
	i686-elf-gcc -I $(INCLUDES) $(FLAGS) -c src/print/print.c -o build/print.o

build/idt/idt.asm.o:  src/idt/idt.asm
	nasm -f elf -g src/idt/idt.asm -o build/idt/idt.asm.o

build/idt/idt.o: src/idt/idt.c
	i686-elf-gcc -I $(INCLUDES) src/idt $(FLAGS) -c src/idt/idt.c -o build/idt/idt.o

build/memory/memory.o: src/memory/memory.c
	i686-elf-gcc -I $(INCLUDES) src/memory $(FLAGS) -c src/memory/memory.c -o build/memory/memory.o

build/io/io.asm.o:  src/io/io.asm
	nasm -f elf -g src/io/io.asm -o build/io/io.asm.o


run:
	qemu-system-x86_64 -drive file=bin/disk.img,index=0,media=disk,format=raw

clean:
	rm -rf bin/boot.bin
	rm -rf bin/kernel.bin
	rm -rf bin/os.bin
	rm -rf build/kernelfull.o
	rm -rf ${MODULES}
	rm -rf bin/disk.img


