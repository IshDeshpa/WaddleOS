BUILD_DIR=./build
SHELL := /bin/bash
DEBUG ?= 0

CROSS_BUILD_DIR=$(BUILD_DIR)/cross
GCC_BUILD_DIR=$(BUILD_DIR)/gcc
BINUTILS_BUILD_DIR=$(BUILD_DIR)/binutils-gdb
KERNEL_BUILD_DIR=$(BUILD_DIR)/kernel
KERNEL_BUILD_SUBDIRS:=$(patsubst kernel%,$(KERNEL_BUILD_DIR)%,$(shell find kernel -type d)) $(KERNEL_BUILD_DIR)/lib
LOADER_BUILD_DIR=$(BUILD_DIR)/loader
BOOT1_BUILD_DIR=$(LOADER_BUILD_DIR)/boot1
BOOT1_BUILD_SUBDIRS=$(BOOT1_BUILD_DIR)/lib
TEST_BUILD_DIR=$(BUILD_DIR)/test

# Cross
TARGET?=x86_64-elf
PREFIX=$(abspath $(CROSS_BUILD_DIR))
CC=$(CROSS_BUILD_DIR)/bin/$(TARGET)-gcc
AS=$(CROSS_BUILD_DIR)/bin/$(TARGET)-as
AR=$(CROSS_BUILD_DIR)/bin/$(TARGET)-ar
LD=$(CROSS_BUILD_DIR)/bin/$(TARGET)-ld
SZ=$(CROSS_BUILD_DIR)/bin/$(TARGET)-size
OBJCOPY=$(CROSS_BUILD_DIR)/bin/$(TARGET)-objcopy
OBJDUMP=$(CROSS_BUILD_DIR)/bin/$(TARGET)-objdump
GCC_STAMP=$(CROSS_BUILD_DIR)/.gcc-built
BINUTILS_STAMP=$(CROSS_BUILD_DIR)/.binutils-built

# CFLAGS
KERNEL_OPT ?= z
LIB_OPT ?= 3
TEST_OPT ?= z
BOOT_OPT ?= z

FREESTANDING_CFLAGS=-ffreestanding -nostdlib
GENERIC_CFLAGS=-std=gnu99 -g -Wall -Wextra -D__x86_64__

# Kernel
KERNEL_C_SRCS   := $(wildcard kernel/*.c) $(wildcard kernel/**/*.c) $(wildcard lib/*.c)
KERNEL_ASM_SRCS := $(wildcard kernel/*.S) $(wildcard kernel/**/*.S) $(wildcard lib/*.S)
KERNEL_C_OBJS   := $(patsubst kernel/%.c,$(KERNEL_BUILD_DIR)/%.o,$(filter kernel/%,$(C_SRCS))) \
            $(patsubst lib/%.c,$(KERNEL_BUILD_DIR)/lib/%.o,$(filter lib/%,$(C_SRCS)))
KERNEL_ASM_OBJS := $(patsubst kernel/%.S,$(KERNEL_BUILD_DIR)/%.o,$(filter kernel/%,$(ASM_SRCS))) \
            $(patsubst lib/%.S,$(KERNEL_BUILD_DIR)/lib/%.o,$(filter lib/%,$(ASM_SRCS)))
KERNEL_OBJS:=$(C_OBJS) $(ASM_OBJS)
KERNEL_C_INCS:=$(foreach dir,$(shell find kernel -type d),-I$(dir)) -Ilib/
KERNEL_CFLAGS:=-O$(KERNEL_OPT) $(GENERIC_CFLAGS) $(FREESTANDING_CFLAGS) -mcmodel=large -mno-red-zone

# Lib
LIB_CFLAGS:=-O$(LIB_OPT) $(GENERIC_CFLAGS) $(FREESTANDING_CFLAGS)

# Test
HOST_CC=gcc
ALL_TEST_DIRS:=$(shell find test/ -mindepth 1 -maxdepth 1 -type d)
ALL_TESTS:=$(notdir $(ALL_TEST_DIRS))

TESTS?=$(ALL_TESTS)
VALID_TESTS:=$(filter $(ALL_TESTS),$(TESTS))
VALID_TEST_DIRS:=$(addprefix test/,$(VALID_TESTS))
VALID_TEST_FILES:=$(foreach d,$(VALID_TEST_DIRS),$(wildcard $(d)/*.c))

TEST_CFLAGS:=-O$(TEST_OPT) $(GENERIC_CFLAGS)
TEST_C_INCS:=$(KERNEL_C_INCS) -Itest/

define GET_TEST_SRCS
$(shell \
    if [ -f $(1)/test.cfg ]; then \
        source $(1)/test.cfg; \
        for f in $$TEST_SRCS; do echo $$f; done; \
    fi)
endef

VALID_TEST_SRCS:=$(foreach d,$(VALID_TEST_DIRS),$(call GET_TEST_SRCS,$d))
TEST_ELF_NAME:=$(subst $(space),_,$(VALID_TESTS))

# Bootloader
BOOT1_C_SRCS := $(wildcard loader/boot1/*.c) $(wildcard lib/*.c)
BOOT1_ASM_SRCS := $(wildcard loader/boot1/*.S) $(wildcard lib/*.S)
BOOT1_C_OBJS   := $(patsubst loader/boot1/%.c,$(BOOT1_BUILD_DIR)/%.o,$(filter loader/boot1/%,$(BOOT1_C_SRCS))) \
            $(patsubst lib/%.c,$(BOOT1_BUILD_DIR)/lib/%.o,$(filter lib/%,$(BOOT1_C_SRCS)))
BOOT1_ASM_OBJS := $(patsubst loader/boot1/%.S,$(BOOT1_BUILD_DIR)/%.o,$(filter loader/boot1/%,$(BOOT1_ASM_SRCS))) \
            $(patsubst lib/%.S,$(BOOT1_BUILD_DIR)/lib/%.o,$(filter lib/%,$(BOOT1_ASM_SRCS)))
BOOT1_OBJS:=$(BOOT1_C_OBJS) $(BOOT1_ASM_OBJS)
BOOT1_C_INCS=-Iloader/boot1/ -Ilib/
BOOT1_CFLAGS=-O$(BOOT_OPT) $(GENERIC_CFLAGS) $(FREESTANDING_CFLAGS) -m32

# QEMU
QEMU_FLAGS=-cpu qemu64
QEMU_SERIAL_OUT?=stdio

ARCH?=64
ifeq ($(ARCH),64)
	QEMU_VER=x86_64
else
	QEMU_VER=i386
endif

ifeq ($(DEBUG),1)
KERNEL_CFLAGS += -DDEBUG
LIB_CFLAGS += -DDEBUG
BOOT1_CFLAGS += -DDEBUG
endif

GDB_ELF ?= $(KERNEL_BUILD_DIR)/kernel.elf

default: all

all: disk 

# Disk
disk: $(BUILD_DIR)/disk.img
	$(SZ) $(LOADER_BUILD_DIR)/boot0.o
	$(SZ) $(LOADER_BUILD_DIR)/boot1.elf
	$(SZ) $(KERNEL_BUILD_DIR)/kernel.elf

$(BUILD_DIR)/disk.img: $(KERNEL_BUILD_DIR)/kernel.elf $(LOADER_BUILD_DIR)/boot0.bin $(LOADER_BUILD_DIR)/boot1.bin
	dd if=$(LOADER_BUILD_DIR)/boot0.bin of=$(BUILD_DIR)/disk.img bs=512 count=1
	dd if=$(LOADER_BUILD_DIR)/boot1.bin of=$(BUILD_DIR)/disk.img bs=512 seek=5
	dd if=$(KERNEL_BUILD_DIR)/kernel.elf of=$(BUILD_DIR)/disk.img bs=512 seek=12

# Kernel
kernel: $(KERNEL_BUILD_DIR)/kernel.elf

$(KERNEL_BUILD_DIR)/%.o: kernel/%.c $(GCC_STAMP)
	@mkdir -p $(@D)
	$(CC) $(KERNEL_CFLAGS) $(KERNEL_C_INCS) -c $< -o $@

$(KERNEL_BUILD_DIR)/%.o: kernel/%.S $(GCC_STAMP)
	@mkdir -p $(@D)
	$(CC) $(KERNEL_CFLAGS) $(KERNEL_C_INCS) -c $< -o $@

$(KERNEL_BUILD_DIR)/lib/%.o: lib/%.c $(GCC_STAMP)
	@mkdir -p $(@D)
	$(CC) $(LIB_CFLAGS) $(KERNEL_C_INCS) -c $< -o $@

$(KERNEL_BUILD_DIR)/lib/%.o: lib/%.S $(GCC_STAMP)
	@mkdir -p $(@D)
	$(CC) $(LIB_CFLAGS) $(KERNEL_C_INCS) -c $< -o $@

$(KERNEL_BUILD_DIR)/kernel.elf: $(KERNEL_OBJS) $(GCC_STAMP)
	@mkdir -p $(@D)
	$(CC) $(KERNEL_CFLAGS) $(KERNEL_C_INCS) -T kernel/linker.ld $(KERNEL_OBJS) -o $@ -lgcc

# Bootloader
loader: $(LOADER_BUILD_DIR)/boot0.bin $(LOADER_BUILD_DIR)/boot1.bin

$(LOADER_BUILD_DIR)/boot0.bin: $(LOADER_BUILD_DIR)/boot0.o $(BINUTILS_STAMP)
	@mkdir -p $(@D)
	$(LD) -m elf_i386 -e _start -Ttext 0x7C00 --oformat binary -o $@ $<

$(LOADER_BUILD_DIR)/boot0.o: loader/boot0.S $(BINUTILS_STAMP)
	@mkdir -p $(@D)
	$(AS) --32 -g -o $@ $<

$(BOOT1_BUILD_DIR)/%.o: loader/boot1/%.c $(GCC_STAMP)
	@mkdir -p $(@D)
	$(CC) $(BOOT1_CFLAGS) $(BOOT1_C_INCS) -c $< -o $@

$(BOOT1_BUILD_DIR)/%.o: loader/boot1/%.S $(GCC_STAMP)
	@mkdir -p $(@D)
	$(CC) $(BOOT1_CFLAGS) $(BOOT1_C_INCS) -c $< -o $@
	
$(BOOT1_BUILD_DIR)/lib/%.o: lib/%.c $(GCC_STAMP)
	@mkdir -p $(@D)
	$(CC) $(LIB_CFLAGS) $(BOOT1_C_INCS) -m32 -c $< -o $@

$(BOOT1_BUILD_DIR)/lib/%.o: lib/%.S $(GCC_STAMP)
	@mkdir -p $(@D)
	$(CC) $(LIB_CFLAGS) $(BOOT1_C_INCS) -m32 -c $< -o $@

$(LOADER_BUILD_DIR)/boot1.elf: $(BOOT1_OBJS) $(BINUTILS_STAMP)
	@mkdir -p $(@D)
	$(LD) $(BOOT1_C_INCS) -m elf_i386 -e _start -T loader/boot1/linker.ld -o $@ $(BOOT1_OBJS)

$(LOADER_BUILD_DIR)/boot1.bin: $(LOADER_BUILD_DIR)/boot1.elf $(BOOT1_OBJS) $(BINUTILS_STAMP)
	@mkdir -p $(@D)
	$(OBJCOPY) -O binary --set-section-flags .rmode=alloc,load,contents  --strip-all $< $@

# Test
test: $(TEST_BUILD_DIR)/$(TEST_ELF_NAME).elf
	@./$(TEST_BUILD_DIR)/$(TEST_ELF_NAME).elf 

$(TEST_BUILD_DIR)/$(TEST_ELF_NAME).elf: $(TEST_BUILD_DIR)/gen/test_main.c $(VALID_TEST_SRCS)
	@mkdir -p $(@D)
	$(HOST_CC) $(TEST_CFLAGS) $(TEST_C_INCS) $(VALID_TEST_FILES) $(VALID_TEST_SRCS) $< -o $@

$(TEST_BUILD_DIR)/gen/test_main.c: $(VALID_TEST_FILES) test/test_main.c.j2
	@mkdir -p $(@D)
	@tmp_file=$@.tmp; \
	uv run python3 test/test_gen.py --cflags '$(TEST_CFLAGS) $(TEST_C_INCS)' --test_sources $(VALID_TEST_FILES) > $$tmp_file; \
	if [ -s $$tmp_file ]; then \
	    mv $$tmp_file $@; \
	else \
	    rm -f $$tmp_file; \
	fi

# For building the proper source files for unit testing
$(TEST_BUILD_DIR)/kernel/%.o: kernel/%.c
	@mkdir -p $(@D)
	$(HOST_CC) $(KERNEL_CFLAGS) $(KERNEL_C_INCS) -c $< -o $@

$(TEST_BUILD_DIR)/kernel/lib/%.o: lib/%.c
	@mkdir -p $(@D)
	$(HOST_CC) $(KERNEL_CFLAGS) $(LIB_CFLAGS) $(KERNEL_C_INCS) -c $< -o $@

# Cross
cross: $(GCC_STAMP) $(BINUTILS_STAMP) 

$(GCC_STAMP): $(BINUTILS_STAMP)
	@mkdir -p $(@D)
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

$(BINUTILS_STAMP): 
	@mkdir -p $(@D)
	export PATH="$(PREFIX)/bin:$$PATH" && \
	pushd $(BINUTILS_BUILD_DIR) && \
		../../binutils-gdb/configure --target="$(TARGET)" --prefix="$(PREFIX)" --with-sysroot --disable-nls --disable-werror --enable-multilib --enable-targets=all && \
	popd && \
	make -j8 -C $(BINUTILS_BUILD_DIR) && \
	make -j8 -C $(BINUTILS_BUILD_DIR) install && \
	touch $@

run:
	qemu-system-$(QEMU_VER) $(QEMU_FLAGS) -drive format=raw,file=build/disk.img -serial $(QEMU_SERIAL_OUT)

debug:
	qemu-system-$(QEMU_VER) $(QEMU_FLAGS) -drive format=raw,file=build/disk.img -S -s -serial $(QEMU_SERIAL_OUT)

gdb:
	$(CROSS_BUILD_DIR)/bin/x86_64-elf-gdb $(GDB_ELF) -ex "target remote localhost:1234"

mon:
	qemu-system-$(QEMU_VER) $(QEMU_FLAGS) -drive format=raw,file=build/disk.img -monitor stdio -no-reboot -no-shutdown

trace:
	qemu-system-$(QEMU_VER) $(QEMU_FLAGS) -drive format=raw,file=build/disk.img -serial $(QEMU_SERIAL_OUT) -d int,cpu_reset -D qemu.log -no-reboot

dump-boot0: $(LOADER_BUILD_DIR)/boot0.o $(BINUTILS_STAMP)
	$(OBJDUMP) -d $< -m i8086

dump-boot1: $(LOADER_BUILD_DIR)/boot1.elf $(BINUTILS_STAMP)
	$(OBJDUMP) -d $< -m i8086

dump-kernel: $(KERNEL_BUILD_DIR)/kernel.elf $(BINUTILS_STAMP)
	$(OBJDUMP) -d $< -m i8086

clean:
	-rm -rf $(KERNEL_BUILD_DIR) $(LOADER_BUILD_DIR) $(LIB_BUILD_DIR) $(TEST_BUILD_DIR)

clean-cross:
	-rm -rf $(CROSS_BUILD_DIR) $(GCC_BUILD_DIR) $(BINUTILS_BUILD_DIR)

.PHONY: all clean run cross kernel gcc binutils loader dump-boot0 dump-boot1 dump-kernel disk mon debug clean-cross gdb lib $(TESTS) test
