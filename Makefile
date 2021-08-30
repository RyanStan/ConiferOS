# 	NOTES
#
# 	bin/disk.img is the flat hard disk that the emulator will see
#
#
# 	what the bin/kernel.bin target recipe is doing:
# 		-the first command links together all our object files into another large intermediary relocatable object file (build/kernelfull.o)
# 		-the second command tells gcc to use our linker script and ld to turn our intermediary relocatable object file (build/kernelfull.o) into a binary at bin/kernel.bin
# 		  using gcc since in the future we will be compiling c files in with this command.  When an object file is passed to gcc, gcc
# 		  skips compilation and instead directly invokes the linker (i686-elf-ld)
#
#

SHELL = /bin/sh
MODULES = build/kernel.asm.o build/kernel.o build/print.o build/idt/idt.asm.o build/idt/idt.o build/memory/memory.o build/io/io.asm.o  build/memory/heap/heap.o build/memory/heap/kernel_heap.o build/memory/paging/paging.o build/memory/paging/paging.asm.o build/disk/disk.o build/string/string.o
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
	nasm -f bin $^ -o $@

build/kernel.asm.o:  src/kernel.asm
	nasm -f elf -g $^ -o $@

build/kernel.o: src/kernel.c
	i686-elf-gcc -I $(INCLUDES) $(FLAGS) -c $^ -o $@

build/print.o: src/print/print.c
	i686-elf-gcc -I $(INCLUDES) $(FLAGS) -c $^ -o $@

build/idt/idt.asm.o:  src/idt/idt.asm
	nasm -f elf -g $^ -o $@

build/idt/idt.o: src/idt/idt.c
	i686-elf-gcc -I $(INCLUDES) src/idt $(FLAGS) -c $^ -o $@

build/memory/memory.o: src/memory/memory.c
	i686-elf-gcc -I $(INCLUDES) src/memory $(FLAGS) -c $^ -o $@

build/io/io.asm.o:  src/io/io.asm
	nasm -f elf -g $^ -o $@

build/memory/heap/heap.o: src/memory/heap/heap.c
	i686-elf-gcc -I $(INCLUDES) src/memory/heap $(FLAGS) -c $^ -o $@

build/memory/heap/kernel_heap.o: src/memory/heap/kernel_heap.c
	i686-elf-gcc -I $(INCLUDES) src/memory/heap $(FLAGS) -c $^ -o $@

build/memory/paging/paging.o: src/memory/paging/paging.c
	i686-elf-gcc -I $(INCLUDES) src/memory/paging $(FLAGS) -c $^ -o $@

build/memory/paging/paging.asm.o:  src/memory/paging/paging.asm
	nasm -f elf -g $^ -o $@

build/disk/disk.o: src/disk/disk.c
	i686-elf-gcc -I $(INCLUDES) src/disk $(FLAGS) -c $^ -o $@

build/string/string.o: src/string/string.c
	i686-elf-gcc -I $(INCLUDES) src/string $(FLAGS) -c $^ -o $@

run:
	qemu-system-i386 -drive file=bin/disk.img,index=0,media=disk,format=raw

clean:
	rm -rf bin/boot.bin
	rm -rf bin/kernel.bin
	rm -rf bin/os.bin
	rm -rf build/kernelfull.o
	rm -rf ${MODULES}
	rm -rf bin/disk.img
	


