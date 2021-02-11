#!/bin/bash 
# add -x switch to above to echo the lines executing

# arg in required. Must be 86 (for 32bit compiler) or 64 (for 64bit compiler)
arch=$1
if [ "x$arch" != "x86" ]; then
   if [ "x$arch" != "x64" ]; then
      echo "One arg in required. Must be 86 (for 32bit compiler) or 64 (for 64bit compiler)"
      exit 1
   fi
fi
if [ "$arch" == "86" ]; then
sz=32
fi
if [ "$arch" == "64" ]; then
sz=64
fi
#echo got sz= $sz and arch= $arch


pushd .
cd ../intel_aes_lib
./mk_lnx_lib${arch}.sh
popd

out=bin/aes_example${arch}
mkdir bin
rm $out

lvl= 
lvl=-O3
yasm=../yasm/yasm

mkdir -p obj/x${arch}

$yasm -D__linux__ -g dwarf2 -f elf${sz} asm/x${arch}/do_rdtsc.s -o obj/x${arch}/do_rdtsc.o

gcc -O3 -m${sz} -g $lvl -o $out src/aes_example.c src/my_getopt.c -Isrc -I../intel_aes_lib/include obj/x${arch}/do_rdtsc.o ../intel_aes_lib/lib/x${arch}/intel_aes${arch}.a

echo created $out
