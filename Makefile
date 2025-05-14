BUILD_DIR=./build

# Cross
CROSS_BUILD_DIR=$(BUILD_DIR)/cross
GCC_BUILD_DIR=$(BUILD_DIR)/gcc
BINUTILS_BUILD_DIR=$(BUILD_DIR)/binutils-gdb
LOADER_BUILD_DIR=$(BUILD_DIR)/loader
TARGET?=x86_64-elf
PREFIX=$(abspath $(CROSS_BUILD_DIR))
CC=$(CROSS_BUILD_DIR)/bin/$(TARGET)-gcc
AS=$(CROSS_BUILD_DIR)/bin/$(TARGET)-as
LD=$(CROSS_BUILD_DIR)/bin/$(TARGET)-ld
GCC_STAMP=$(CROSS_BUILD_DIR)/.gcc-built
BINUTILS_STAMP=$(CROSS_BUILD_DIR)/.binutils-built

# Kernel
KERNEL_BUILD_DIR=$(BUILD_DIR)/kernel
C_SRCS=$(wildcard src/*.c)
ASM_SRCS=$(wildcard src/*.S)
C_OBJS:=$(addprefix $(KERNEL_BUILD_DIR)/, $(notdir $(C_SRCS:.c=.o)))
ASM_OBJS:=$(addprefix $(KERNEL_BUILD_DIR)/, $(notdir $(ASM_SRCS:.S=.o)))
OBJS:=$(C_OBJS) $(ASM_OBJS)
CFLAGS=-std=gnu99 -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -g 

default: all

all: kernel loader 
cross: $(GCC_STAMP) $(BINUTILS_STAMP) 

# Kernel
kernel: $(KERNEL_BUILD_DIR)/kernel.elf

$(KERNEL_BUILD_DIR)/%.o: src/%.c $(GCC_STAMP) | $(KERNEL_BUILD_DIR) 
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BUILD_DIR)/%.o: src/%.S $(GCC_STAMP) | $(KERNEL_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BUILD_DIR)/kernel.elf: $(OBJS) $(GCC_STAMP) | $(KERNEL_BUILD_DIR)
	$(CC) $(CFLAGS) -nostdlib -T linker.ld $(OBJS) -o $@ -lgcc

# Bootloader
loader: $(LOADER_BUILD_DIR)/loader.bin
	
$(LOADER_BUILD_DIR)/loader.bin: $(LOADER_BUILD_DIR)/loader.o $(GCC_STAMP) | $(LOADER_BUILD_DIR)
	$(LD) -m elf_i386 -Ttext 0x7C00 -e _start --oformat binary -o $@ $<

$(LOADER_BUILD_DIR)/loader.o: loader/loader.S $(GCC_STAMP) | $(LOADER_BUILD_DIR)
	$(AS) --32 -o $@ $<

# Cross
$(GCC_STAMP): $(GCC_BUILD_DIR) $(BINUTILS_STAMP)
	export PATH="$(PREFIX)/bin:$$PATH" && \
	pushd gcc && \
	./contrib/download_prerequisites && \
	popd && \
	pushd $(GCC_BUILD_DIR) && \
	../../gcc/configure --target="$(TARGET)" --prefix="$(PREFIX)" --disable-nls --enable-languages=c,c++ --enable-multilib --without-headers --disable-hosted-libstdcxx && \
	popd && \
	make -j8 -C $(GCC_BUILD_DIR) all-gcc && \
	make -j8 -C $(GCC_BUILD_DIR) all-target-libgcc && \
	make -j8 -C $(GCC_BUILD_DIR) all-target-libstdc++-v3 && \
	make -j8 -C $(GCC_BUILD_DIR) install-gcc && \
	make -j8 -C $(GCC_BUILD_DIR) install-target-libgcc && \
	make -j8 -C $(GCC_BUILD_DIR) install-target-libstdc++-v3 && \
	touch $@

$(BINUTILS_STAMP): $(BINUTILS_BUILD_DIR)
	export PATH="$(PREFIX)/bin:$$PATH" && \
	pushd $(BINUTILS_BUILD_DIR) && \
		../../binutils-gdb/configure --target="$(TARGET)" --prefix="$(PREFIX)" --with-sysroot --disable-nls --disable-werror --enable-multilib && \
	popd && \
	make -j8 -C $(BINUTILS_BUILD_DIR) && \
	make -j8 -C $(BINUTILS_BUILD_DIR) install && \
	touch $@

# Build dirs
$(BUILD_DIR) $(KERNEL_BUILD_DIR) $(GCC_BUILD_DIR) $(BINUTILS_BUILD_DIR) $(LOADER_BUILD_DIR):
	mkdir -p $@

run:
	qemu-system-x86_64 -kernel $(KERNEL_BUILD_DIR)/kernel.elf 

clean:
	-rm -rf $(KERNEL_BUILD_DIR) $(LOADER_BUILD_DIR)

clean-cross:
	-rm -rf $(CROSS_BUILD_DIR) $(GCC_BUILD_DIR) $(BINUTILS_BUILD_DIR)

.PHONY: all clean run cross kernel gcc binutils
