all:
	nasm -f elf32 kernel.asm -o kasm.o
	gcc -m32 -fno-stack-protector -c kernel.c -o kc.o
	ld -m elf_i386 -T link.ld -o kernel kasm.o kc.o 

#64bit:
#	nasm -f elf64 kernel.asm -o kasm.o
#	gcc -m64 -c -fno-stack-protector kernel.c -o kc.o
#	ld -m link.ld -o kernel-64 kasm.o kc.o

run:
	qemu-system-i386 -kernel kernel

#debug-64:
#	make 64bit
#	qemu-system-x86_64 -kernel kernel-64

export:
	sudo cp kernel kernel-701
	sudo cp kernel-701 /boot
	sudo update-grub2
