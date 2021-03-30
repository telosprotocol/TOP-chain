#!/bin/bash -x

arch=86

if [ "$arch" == "86" ]; then 
sz=32
asm=aes_x86_v2
fi

if [ "$arch" == "64" ]; then 
sz=64
asm=aes_amd64
fi

mkdir bin
mkdir obj/x${arch}

yasm=../yasm/yasm
for i in $asm; do echo do $i.asm; $yasm -D__GNUC__ -D__linux__ -g dwarf2 -f elf${sz} asm/x${arch}/$i.asm -o obj/x${arch}/$i.o; done

opt="-O3 -I../intel_aes_lib/include"
files="aeskey aesaux aes_modes modetest process_options my_getopt"
set obs=
for i in $files; do
	obj=obj/x$arch/$i.o
	obs="$obs $obj"
	gcc -m$sz -g $opt -Iinclude -o $obj -c src/$i.c
	if [ $? -ne 0 ]
	then
		echo "Got error compiling src/$i.c"
		exit
	fi
done

obs="$obs obj/x$arch/$asm.o"
gcc -m$sz -g $opt -o bin/modetest$arch  $obs ../intel_aes_lib/lib/x$arch/intel_aes$arch.a -lm -lrt
if [ $? -ne 0 ]
then
	echo "Got error on link"
	exit
fi

