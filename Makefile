BUILD_DIR=./build

# Cross
CROSS_BUILD_DIR=$(BUILD_DIR)/cross
GCC_BUILD_DIR=$(BUILD_DIR)/gcc
BINUTILS_BUILD_DIR=$(BUILD_DIR)/binutils-gdb
TARGET?=x86_64-elf
PREFIX=$(abspath $(CROSS_BUILD_DIR))
CC=$(CROSS_BUILD_DIR)/bin/$(TARGET)-gcc

# Kernel
KERNEL_BUILD_DIR=$(BUILD_DIR)/kernel
C_SRCS=$(wildcard src/*.c)
ASM_SRCS=$(wildcard src/*.s)
C_OBJS:=$(addprefix $(KERNEL_BUILD_DIR)/, $(notdir $(C_SRCS:.c=.o)))
ASM_OBJS:=$(addprefix $(KERNEL_BUILD_DIR)/, $(notdir $(ASM_SRCS:.s=.o)))
OBJS:=$(C_OBJS) $(ASM_OBJS)
CFLAGS=-std=gnu99 -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -g 


$(info $(OBJS))
$(info $(KERNEL_BUILD_DIR))

default: all

all: kernel 
cross: gcc binutils

# Kernel
kernel: $(KERNEL_BUILD_DIR)/kernel.elf

$(KERNEL_BUILD_DIR)/%.o: src/%.c $(CC) | $(KERNEL_BUILD_DIR) 
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BUILD_DIR)/%.o: src/%.s $(CC) | $(KERNEL_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BUILD_DIR)/kernel.elf: $(OBJS) $(CC) | $(KERNEL_BUILD_DIR)
	$(CC) $(CFLAGS) -nostdlib -T linker.ld $^ -o $@ -lgcc

$(CC): cross

# Cross
gcc: $(GCC_BUILD_DIR)
	pushd $(GCC_BUILD_DIR) && \
	../../gcc/configure --target="$(TARGET)" --prefix="$(PREFIX)" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx && \
		popd 
	make -j8 -C $(GCC_BUILD_DIR) all-gcc
	make -j8 -C $(GCC_BUILD_DIR) all-target-libgcc
	make -j8 -C $(GCC_BUILD_DIR) all-target-libstdc++-v3
	make -j8 -C $(GCC_BUILD_DIR) install-gcc
	make -j8 -C $(GCC_BUILD_DIR) install-target-libgcc
	make -j8 -C $(GCC_BUILD_DIR) install-target-libstdc++-v3

binutils: $(BINUTILS_BUILD_DIR)
	pushd $(BINUTILS_BUILD_DIR) && \
		../../binutils-gdb/configure --target="$(TARGET)" --prefix="$(PREFIX)" --with-sysroot --disable-nls --disable-werror && \
		popd
	make -j8 -C $(BINUTILS_BUILD_DIR)
	make -j8 -C $(BINUTILS_BUILD_DIR) install

# Build dirs
$(BUILD_DIR) $(KERNEL_BUILD_DIR) $(GCC_BUILD_DIR) $(BINUTILS_BUILD_DIR):
	mkdir -p $@

run:
	qemu-system-i386 -machine type=pc-i440fx-3.1 -kernel $(BUILD_DIR)/kernel.elf 



clean:
	-rm -rf $(KERNEL_BUILD_DIR)

clean-cross:
	-rm -rf $(CROSS_BUILD_DIR) $(GCC_BUILD_DIR) $(BINUTILS_BUILD_DIR)

.PHONY: all clean run cross binutils gcc kernel
