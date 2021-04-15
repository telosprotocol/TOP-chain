#!/bin/bash
source /etc/profile
set -x

BUILD_ARGS=$1

tmp=$(echo ${BUILD_ARGS}|sed $'s/\,/ /g')
echo ${tmp}
fix_args=$(echo ${tmp}|sed $'s/\"//g')

echo "build args: "${fix_args}

./build.sh ${fix_args}

if [ ! -f "cbuild/bin/Linux/xtopchain" ];then
    echo "build failed!!!"
    exit -1
fi

xtop_md5=$(md5sum cbuild/bin/Linux/xtopchain)
echo "build success, xtopchain md5: "${xtop_md5}
