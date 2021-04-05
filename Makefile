BOOT_ASM = src/boot/boot.asm
BOOT_BIN = bin/boot.bin

boot.bin: $(BOOT_ASM)
	nasm -f bin $(BOOT_ASM) -o $(BOOT_BIN)

run:
	qemu-system-x86_64 -hda $(BOOT_BIN)

clean:
	rm -rf $(BOOT_BIN)

hex: 
	bless $(BOOT_BIN)
