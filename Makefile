CC=./build/cross/bin/i686-elf-gcc
BUILD_DIR=./build
C_SRCS=$(wildcard *.c) 
ASM_SRCS=$(wildcard *.s)
OBJS=$(C_SRCS:%.c=$(BUILD_DIR)/%.o) $(ASM_SRCS:%.s=$(BUILD_DIR)/%.o)

default: all

all: $(BUILD_DIR)/kernel.elf

$(BUILD_DIR)/%.o: %.c
	$(CC) -std=gnu99 -ffreestanding -g -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	$(CC) -std=gnu99 -ffreestanding -g -c $< -o $@

$(BUILD_DIR)/kernel.elf: $(OBJS)
	$(CC) -ffreestanding -nostdlib -g -T linker.ld $^ -o $@ -lgcc

run:
	qemu-system-i386 -machine type=pc-i440fx-3.1 -kernel $(BUILD_DIR)/kernel.elf 


clean:
	-rm -rf build/*.o build/*.elf

.PHONY: all clean
