#!/bin/bash
source /etc/profile
source ~/.bashrc
# set -x

BUILD_ARGS=$1

tmp=$(echo ${BUILD_ARGS}|sed $'s/\,/ /g')
echo ${tmp}
fix_args=$(echo ${tmp}|sed $'s/\"//g')

echo "build args: "${fix_args}

./build.sh ${fix_args}

if [ $? -ne 0 ];then
    echo "build fail!"
    exit -1
fi

if [ ! -f "cbuild/bin/Linux/topio" ];then
    echo "build topio failed!!!"
    exit -1
fi

topio_md5=$(md5sum cbuild/lib/Linux/topio)
echo "build success, topio md5: "${topio_md5}
