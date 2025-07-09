# Makefile LLD/Clang
CC      = clang
AS      = clang
CFLAGS  = --target=aarch64-elf -march=armv8-a -ffreestanding -nostdlib -Iinclude
LDFLAGS = -fuse-ld=lld -T linker.ld

OBJS = boot.o enter_usermode.o kernel.o uart.o ramfs.o exceptions.o exceptions_c.o timer.o gic.o mmu.o process.o context_switch.o process_test.o vfs.o kmalloc.o string.o abyssfs.o message.o namespace.o shell.o uart_debug.o user_shell.o

all: kernel.elf

boot.o: src/arch/boot.S
	$(AS) $(CFLAGS) -c src/arch/boot.S -o boot.o

enter_usermode.o: src/arch/enter_usermode.S
	$(AS) $(CFLAGS) -c src/arch/enter_usermode.S -o enter_usermode.o

kernel.o: src/kernel/kernel.c
	$(CC) $(CFLAGS) -c src/kernel/kernel.c -o kernel.o

uart.o: src/drivers/uart.c
	$(CC) $(CFLAGS) -c src/drivers/uart.c -o uart.o

ramfs.o: src/fs/ramfs.c
	$(CC) $(CFLAGS) -c src/fs/ramfs.c -o ramfs.o

exceptions.o: src/arch/exceptions.S
	$(AS) $(CFLAGS) -c src/arch/exceptions.S -o exceptions.o

exceptions_c.o: src/arch/exceptions.c
	$(CC) $(CFLAGS) -c src/arch/exceptions.c -o exceptions_c.o

timer.o: src/drivers/timer.c
	$(CC) $(CFLAGS) -c src/drivers/timer.c -o timer.o

gic.o: src/drivers/gic.c
	$(CC) $(CFLAGS) -c src/drivers/gic.c -o gic.o

mmu.o: src/arch/mmu.c
	$(CC) $(CFLAGS) -c src/arch/mmu.c -o mmu.o

process.o: src/kernel/process.c
	$(CC) $(CFLAGS) -c src/kernel/process.c -o process.o

context_switch.o: src/arch/context_switch.S
	$(AS) $(CFLAGS) -c src/arch/context_switch.S -o context_switch.o

process_test.o: src/user/process_test.c
	$(CC) $(CFLAGS) -c src/user/process_test.c -o process_test.o

vfs.o: src/fs/vfs.c
	$(CC) $(CFLAGS) -c src/fs/vfs.c -o vfs.o

kmalloc.o: src/mm/kmalloc.c
	$(CC) $(CFLAGS) -c src/mm/kmalloc.c -o kmalloc.o

string.o: src/mm/string.c
	$(CC) $(CFLAGS) -c src/mm/string.c -o string.o

abyssfs.o: src/fs/abyssfs.c
	$(CC) $(CFLAGS) -c src/fs/abyssfs.c -o abyssfs.o

message.o: src/kernel/message.c
	$(CC) $(CFLAGS) -c src/kernel/message.c -o message.o

namespace.o: src/kernel/namespace.c
	$(CC) $(CFLAGS) -c src/kernel/namespace.c -o namespace.o

shell.o: src/user/shell.c
	$(CC) $(CFLAGS) -c src/user/shell.c -o shell.o

user_stub.o: user_stub.S
	$(AS) $(CFLAGS) -c $< -o $@

user_shell.o: src/arch/user_shell.S
	$(AS) $(CFLAGS) -c src/arch/user_shell.S -o user_shell.o

uart_debug.o: src/drivers/uart_debug.c
	$(CC) $(CFLAGS) -c src/drivers/uart_debug.c -o uart_debug.o

kernel.elf: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o kernel.elf

clean:
	rm -f *.o kernel.elf