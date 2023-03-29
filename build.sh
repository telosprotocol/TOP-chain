#!/bin/bash

command_line_option_include_item() {
    local list="$1"
    local value="$2"
    local result=1

    for item in $list
    do
        if [ $item = $value ]; then
            # found
            result=0
        fi;
    done
    return $result
}

osname=`uname` # Linux/Darwin
echo "osname: $osname"

distrib=`cat /etc/*-release | uniq -u | grep ^ID= | grep -oP "ID=\"*\K\w+"`
echo "distribution: $distrib"

CMAKE_EXE=`which cmake || which cmake3`
if [ ! -f $CMAKE_EXE ]; then
    echo "cmake not installed"
    exit
fi

options="$@"

command_line_option_include_item "$options" "pack"
if [ $? -eq 0 ]; then
    # make install package
    pack_mode="debug"
    command_line_option_include_item "$options" "release"
    if [ $? -eq 0 ]; then
        pack_mode="release"
    fi
    sh ./pack.sh "$pack_mode"
    exit 0
fi

command_line_option_include_item "$options" "install"
if [ $? -eq 1 ]; then
    echo "no install found, compile mode"
    CMAKE_EXTRA_OPTIONS="-DCMAKE_BUILD_TYPE=Debug"
    CBUILD_DIR="cbuild"

    source ./build_options.sh

    command_line_option_include_item "$options" "leak_trace"
    if [ $? -eq 0 ]; then
        CBUILD_DIR="${CBUILD_DIR}_leak"
    fi

    echo "CMAKE_EXTRA_OPTIONS: ${CMAKE_EXTRA_OPTIONS}"
    echo "CBUILD_DIR: ${CBUILD_DIR}"

    mkdir -p ${CBUILD_DIR}
    cd ${CBUILD_DIR}
    printf "$options" > ./build_options.inc

    if [ $osname == "Linux" ]; then
        $CMAKE_EXE ${CMAKE_EXTRA_OPTIONS} ..
        CPU_CORE=$( lscpu -pCPU | grep -v "#" | wc -l )
    elif [ $osname == "Darwin" ]; then
        $CMAKE_EXE .. ${CMAKE_EXTRA_OPTIONS}
        CPU_CORE=$( sysctl hw|grep ncpu|awk -F ':' '{print $2}' )
    fi

    # # MEM_MEG=$( free -m | sed -n 2p | tr -s ' ' | cut -d\  -f2 )

    # # MEM_GIG=$(( ((MEM_MEG / 1000) / 2) ))
    # # JOBS=$(( MEM_GIG > CPU_CORE ? CPU_CORE : MEM_GIG ))
    # # make -j${JOBS}
    if [ $CPU_CORE -le 4 ]; then
        make -j4
    else
        make -j${CPU_CORE}
    fi
else
    cbuild_path="cbuild"
    command_line_option_include_item "$options" "release"
    if [ $? -eq 0 ]; then
        cbuild_path="cbuild_release"
    fi

    echo "install found, install mode from path:$cbuild_path"
    cd $cbuild_path
    printf "$options" > ./build_options.inc
    make install
    sudo ldconfig
fi
