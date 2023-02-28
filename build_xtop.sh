#!/bin/bash
source /etc/profile
source ~/.bashrc
# set -x

build_mode=""

get_build_mode() {
    local args="$1"
    local mode=""
    local counter=0

    for item in $args
    do
        if [ $item == "release" ]; then
            mode="release"
            let counter++
        fi
    done

    for item in $args
    do
        if [ $item == "reldbg" ]; then
            mode="reldbg"
            let counter++
        fi
    done

    for item in $args
    do
        if [ $item == "debug" ]; then
            mode="debug"
            let counter++
        fi
    done

    if [ $counter -eq 0 ]; then
        mode="debug"
    elif [ $counter -gt 1 ]; then
        mode=""
        echo "build with multiple modes is not supported"
    fi

    build_mode=$mode
}

BUILD_ARGS=$1

tmp=$(echo ${BUILD_ARGS}|sed $'s/\,/ /g')
echo ${tmp}
fix_args=$(echo ${tmp}|sed $'s/\"//g')

echo "build args: "${fix_args}
get_build_mode "${fix_args}"
if [ "x$build_mode" == "x" ]; then
    exit -1
fi

./build.sh ${fix_args}

if [ $? -ne 0 ];then
    echo "build fail!"
    exit -1
fi

if [ $build_mode == "debug" ]; then
    if [ ! -f "cbuild/bin/Linux/topio" ]; then
        echo "build topio failed!!!"
        exit -1
    else
        topio_md5=$(md5sum cbuild/bin/Linux/topio)
        echo "build success, topio md5: "${topio_md5}
    fi
fi

if [ $build_mode == "release" ]; then
    if [ ! -f "cbuild_release/bin/Linux/topio" ]; then
        echo "build topio failed!!!"
        exit -1
    else
        topio_md5=$(md5sum cbuild_release/bin/Linux/topio)
        echo "build success, topio md5: "${topio_md5}
    fi
fi

if [ $build_mode == "reldbg" ]; then
    if [ ! -f "cbuild_reldbg/bin/Linux/topio" ]; then
        echo "build topio failed!!!"
        exit -1
    else
        topio_md5=$(md5sum cbuild_reldbg/bin/Linux/topio)
        echo "build success, topio md5: "${topio_md5}
    fi
fi
