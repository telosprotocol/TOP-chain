#!/bin/bash

pushd .
cd ../intel_aes_lib
./mk_lnx_lib86.sh
popd

out=bin/aes_example86
mkdir bin
rm $out

lvl= 
lvl=-O3
yasm=../yasm/yasm

mkdir -p obj/x86

gcc -m32 -g $lvl -o $out src/aes_example.c src/my_getopt.c -Isrc -I../intel_aes_lib/include  ../intel_aes_lib/lib/x86/intel_aes86.a

echo created $out
