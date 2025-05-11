#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

pushd $SCRIPT_DIR

if [ ! -d "gcc" ]; then
  git clone git@github.com:gcc-mirror/gcc.git
else
  pushd gcc
  git pull
  git checkout releases/gcc-15
  popd
fi

if [ ! -d "gcc" ]; then
  git clone git://sourceware.org/git/binutils-gdb.git
else
  pushd binutils-gdb
  git pull
  popd
fi

popd
