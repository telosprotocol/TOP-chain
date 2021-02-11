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
asm=aes_x86_v2
src="aeskey aesaux aes_modes modetest process_options my_getopt"
defines="-D ASM_X86_V2"
fi
if [ "$arch" == "64" ]; then
sz=64
asm=aes_amd64
src="aeskey aestab aesaux aes_modes modetest process_options my_getopt"
defines="-D ASM_AMD64_C"
fi
#echo got sz= $sz and arch= $arch


pushd .
cd ../intel_aes_lib
./mk_lnx_lib${arch}.sh
popd

out=bin/modetest$arch
mkdir bin
rm $out

mkdir -p obj/x${arch}

yasm=../yasm/yasm
for i in $asm; do echo do $i.asm; $yasm -D__GNUC__ -D__linux__ -g dwarf2 -f elf${sz} asm/x${arch}/$i.asm -o obj/x${arch}/$i.o; done

lvl="-fPIC -O3 -I../intel_aes_lib/include"
set obs=
for i in $src; do
	obj=obj/x$arch/$i.o
	obs="$obs $obj"
	gcc -m$sz -g $lvl $defines -Iinclude -o $obj -c src/$i.c
	if [ $? -ne 0 ]
	then
		echo "Got error compiling src/$i.c"
		exit
	fi
done

obs="$obs obj/x$arch/$asm.o"
gcc -m$sz -g $lvl -o $out  $obs ../intel_aes_lib/lib/x$arch/intel_aes$arch.a -lm -lrt
if [ $? -ne 0 ]
then
	echo "Got error on link"
	exit
fi

echo created $out
