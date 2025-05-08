#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

source $SCRIPT_DIR/.env

mkdir -p $SCRIPT_DIR/build/build-binutils
pushd $SCRIPT_DIR/build/build-binutils
../../binutils-gdb/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make -j8
make -j8 install
popd

mkdir -p $SCRIPT_DIR/build/build-gcc
pushd $SCRIPT_DIR/build/build-gcc
../../gcc/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx
make -j8 all-gcc
make -j8 all-target-libgcc
make -j8 all-target-libstdc++-v3
make -j8 install-gcc
make -j8 install-target-libgcc
make -j8 install-target-libstdc++-v3
popd
