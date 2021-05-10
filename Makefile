bin/disk.img: bin/boot.bin
	cp bin/boot.bin bin/disk.img

bin/boot.bin: src/boot/boot.asm
	nasm -f bin src/boot/boot.asm -o bin/boot.bin

run:
	qemu-system-x86_64 -drive file=bin/disk.img,index=0,media=disk,format=raw

clean:
	rm -rf bin/boot.bin

