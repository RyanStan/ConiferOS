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

SHELL = /bin/sh
MODULES = build/kernel.asm.o

#TODO: add environment variables from build.sh so I don't need to use that script

all: bin/boot.bin bin/kernel.bin
	dd if=bin/boot.bin > bin/os.bin
	dd if=bin/kernel.bin >> bin/os.bin
	dd if=/dev/zero bs=512 count=100 >> bin/os.bin # Fills up rest of disk with 100 sector sized blocks of zeros (we'll be loading our kernel in eventually over these null sectors)

bin/kernel.bin: $(MODULES)
	i686-elf-ld -g -relocatable $(MODULES) -o build/kernelfull.o
	i686-elf-gcc -T src/linker.ld -o bin/kernel.bin -ffreestanding -O0 -nostdlib build/kernelfull.o
	
bin/boot.bin: src/boot/boot.asm
	nasm -f bin src/boot/boot.asm -o bin/boot.bin

build/kernel.asm.o:  src/kernel.asm
	nasm -f elf -g src/kernel.asm -o build/kernel.asm.o

run:
	qemu-system-x86_64 -hda bin/boot.bin

clean:
	rm -rf bin/boot.bin
	rm -rf bin/os.bin
	rm -rf build/kernel.asm.o
	rm -rf build/kernelfull.o


