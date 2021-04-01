all: boot.asm
	nasm -f bin boot.asm -o boot.bin
	dd if=message.txt >> boot.bin # Append message to 2nd sector of boot.bin disk 
	dd if=/dev/zero bs=512 count=1 >> boot.bin # Make sure that 2nd sector is full

run:
	qemu-system-x86_64 -hda boot.bin

hex: 
	bless boot.bin
