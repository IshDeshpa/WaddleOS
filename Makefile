BUILD_DIR=./build

DEBUG ?= 0

# Cross
CROSS_BUILD_DIR=$(BUILD_DIR)/cross
GCC_BUILD_DIR=$(BUILD_DIR)/gcc
BINUTILS_BUILD_DIR=$(BUILD_DIR)/binutils-gdb
TARGET?=x86_64-elf
PREFIX=$(abspath $(CROSS_BUILD_DIR))
CC=$(CROSS_BUILD_DIR)/bin/$(TARGET)-gcc
AS=$(CROSS_BUILD_DIR)/bin/$(TARGET)-as
LD=$(CROSS_BUILD_DIR)/bin/$(TARGET)-ld
OBJCOPY=$(CROSS_BUILD_DIR)/bin/$(TARGET)-objcopy
OBJDUMP=$(CROSS_BUILD_DIR)/bin/$(TARGET)-objdump
GCC_STAMP=$(CROSS_BUILD_DIR)/.gcc-built
BINUTILS_STAMP=$(CROSS_BUILD_DIR)/.binutils-built

# Kernel
KERNEL_BUILD_DIR=$(BUILD_DIR)/kernel
C_SRCS=$(wildcard kernel/*.c)
ASM_SRCS=$(wildcard kernel/*.S)
C_OBJS:=$(addprefix $(KERNEL_BUILD_DIR)/, $(notdir $(C_SRCS:.c=.o)))
ASM_OBJS:=$(addprefix $(KERNEL_BUILD_DIR)/, $(notdir $(ASM_SRCS:.S=.o)))
OBJS:=$(C_OBJS) $(ASM_OBJS)
CINC=-Ikernel/
CFLAGS=-std=gnu99 -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -g -Wall -Wextra -nostdlib
CDEFS=-D__x86_64__

# Loader
LOADER_BUILD_DIR=$(BUILD_DIR)/loader
BOOT1_BUILD_DIR=$(LOADER_BUILD_DIR)/boot1
BOOT1_C_SRCS=$(wildcard loader/boot1/*.c)
BOOT1_ASM_SRCS=$(wildcard loader/boot1/*.S)
BOOT1_C_OBJS:=$(addprefix $(BOOT1_BUILD_DIR)/, $(notdir $(BOOT1_C_SRCS:.c=.o)))
BOOT1_ASM_OBJS:=$(addprefix $(BOOT1_BUILD_DIR)/, $(notdir $(BOOT1_ASM_SRCS:.S=.o)))
BOOT1_OBJS:=$(BOOT1_C_OBJS) $(BOOT1_ASM_OBJS)
BOOT1_CINC=-Iloader/boot1/
BOOT1_CFLAGS=-std=gnu99 -ffreestanding -m32 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -g -Wall -Wextra -nostdlib -Oz

# QEMU
QEMU_FLAGS=-cpu qemu64 -M pc

ARCH?=64
ifeq ($(ARCH),64)
	QEMU_VER=x86_64
else
	QEMU_VER=i386
endif

ifeq ($(DEBUG),1)
CDEFS        += -DDEBUG
BOOT1_CFLAGS += -DDEBUG
endif

default: all

all: disk 

# Disk
disk: $(BUILD_DIR)/disk.img

$(BUILD_DIR)/disk.img: kernel loader
	dd if=$(LOADER_BUILD_DIR)/boot0.bin of=$(BUILD_DIR)/disk.img bs=512 count=1
	dd if=$(LOADER_BUILD_DIR)/boot1.bin of=$(BUILD_DIR)/disk.img bs=512 seek=5
	dd if=$(KERNEL_BUILD_DIR)/kernel.elf of=$(BUILD_DIR)/disk.img bs=512 seek=12

# Kernel
kernel: $(KERNEL_BUILD_DIR)/kernel.elf

$(KERNEL_BUILD_DIR)/%.o: kernel/%.c $(GCC_STAMP) | $(KERNEL_BUILD_DIR) 
	$(CC) $(CFLAGS) $(CDEFS) $(CINC) -c $< -o $@

$(KERNEL_BUILD_DIR)/%.o: kernel/%.S $(GCC_STAMP) | $(KERNEL_BUILD_DIR)
	$(CC) $(CFLAGS) $(CDEFS) $(CINC) -c $< -o $@

$(KERNEL_BUILD_DIR)/kernel.elf: $(OBJS) $(GCC_STAMP) | $(KERNEL_BUILD_DIR)
	$(CC) $(CFLAGS) $(CDEFS) $(CINC) -T kernel/linker.ld $(OBJS) -o $@ -lgcc

# Bootloader
loader: $(LOADER_BUILD_DIR)/boot0.bin $(LOADER_BUILD_DIR)/boot1.bin | $(LOADER_BUILD_DIR)

$(LOADER_BUILD_DIR)/boot0.bin: $(LOADER_BUILD_DIR)/boot0.o | $(LOADER_BUILD_DIR)
	$(LD) -m elf_i386 -e _start -Ttext 0x7C00 --oformat binary -o $@ $<

$(LOADER_BUILD_DIR)/boot0.o: loader/boot0.S | $(LOADER_BUILD_DIR)
	$(AS) --32 -g -o $@ $<

$(BOOT1_BUILD_DIR)/%.o: loader/boot1/%.c | $(BOOT1_BUILD_DIR)
	$(CC) $(BOOT1_CFLAGS) $(BOOT1_CINC) -c $< -o $@

$(BOOT1_BUILD_DIR)/%.o: loader/boot1/%.S | $(BOOT1_BUILD_DIR)
	$(CC) $(BOOT1_CFLAGS) $(BOOT1_CINC) -m32 -g -c -o $@ $<

$(LOADER_BUILD_DIR)/boot1.elf: $(BOOT1_OBJS) | $(LOADER_BUILD_DIR)
	$(LD) $(BOOT1_CINC) -m elf_i386 -e _start -T loader/boot1/linker.ld -o $@ $(BOOT1_OBJS)

$(LOADER_BUILD_DIR)/boot1.bin: $(LOADER_BUILD_DIR)/boot1.elf $(BOOT1_OBJS) | $(LOADER_BUILD_DIR)
	$(OBJCOPY) -O binary --set-section-flags .rmode=alloc,load,contents  --strip-all $< $@
	
# Cross
cross: $(GCC_STAMP) $(BINUTILS_STAMP) 

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
		../../binutils-gdb/configure --target="$(TARGET)" --prefix="$(PREFIX)" --with-sysroot --disable-nls --disable-werror --enable-multilib --enable-targets=all && \
	popd && \
	make -j8 -C $(BINUTILS_BUILD_DIR) && \
	make -j8 -C $(BINUTILS_BUILD_DIR) install && \
	touch $@

# Build dirs
$(BUILD_DIR) $(KERNEL_BUILD_DIR) $(GCC_BUILD_DIR) $(BINUTILS_BUILD_DIR) $(LOADER_BUILD_DIR) $(BOOT1_BUILD_DIR):
	mkdir -p $@

run:
	qemu-system-$(QEMU_VER) $(QEMU_FLAGS) -drive format=raw,file=build/disk.img

debug:
	qemu-system-$(QEMU_VER) $(QEMU_FLAGS) -drive format=raw,file=build/disk.img -S -s

gdb:
	$(CROSS_BUILD_DIR)/bin/x86_64-elf-gdb $(KERNEL_BUILD_DIR)/kernel.elf

mon:
	qemu-system-$(QEMU_VER) $(QEMU_FLAGS) -drive format=raw,file=build/disk.img -monitor stdio

trace:
	qemu-system-$(QEMU_VER) $(QEMU_FLAGS) -drive format=raw,file=build/disk.img -d cpu,exec,in_asm -D qemu.log

dump0: $(LOADER_BUILD_DIR)/boot0.o $(BINUTILS_STAMP)
	$(OBJDUMP) -d $< -m i8086

dump1: $(LOADER_BUILD_DIR)/boot1.elf $(BINUTILS_STAMP)
	$(OBJDUMP) -d $< -m i8086

clean:
	-rm -rf $(KERNEL_BUILD_DIR) $(LOADER_BUILD_DIR)

clean-cross:
	-rm -rf $(CROSS_BUILD_DIR) $(GCC_BUILD_DIR) $(BINUTILS_BUILD_DIR)

.PHONY: all clean run cross kernel gcc binutils loader dump0 dump1 disk mon debug clean-cross gdb
