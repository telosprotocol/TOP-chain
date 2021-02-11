#!/bin/sh



echo_and_run() { echo "$*" ; "$@" ; }

debug_release_mode="debug"
if [  $1 ]; then
    debug_release_mode=$1
fi

echo ""
echo "######################begin tar topio-package in ${debug_release_mode} mode#############################"
echo ""


topio_version="0.0.0.0"

if [ "debug" == $debug_release_mode ]; then
    if [ ! -d "cbuild" ]; then
        echo "make sure dir cbuild exists"
        exit -1
    fi

    topio_version=`cat cbuild/generated/version.h  |grep PROGRAM_VERSION |awk -F '\"' '{print $2}' `
elif [ "release" == $debug_release_mode ]; then
    if [ ! -d "cbuild_release" ]; then
        echo "make sure dir cbuild_release exists"
        exit -1
    fi

    topio_version=`cat cbuild_release/generated/version.h  |grep PROGRAM_VERSION |awk -F '\"' '{print $2}' `
else
    echo "not support params,usage: pack.sh [debug|release]"
    exit -1
fi

debug_build_topio_path="./cbuild/bin/Linux/topio"
debug_build_libxtopchain_so_path="./cbuild/lib/Linux/libxtopchain.so.$topio_version"

release_build_topio_path="./cbuild_release/bin/Linux/topio"
release_build_libxtopchain_so_path="./cbuild_release/lib/Linux/libxtopchain.so.$topio_version"

topio_install_sh_path="./src/xtopcom/xnode/install.sh"
topio_uninstall_sh_path="./src/xtopcom/xnode/uninstall.sh"

topio_readme_path="./src/xtopcom/xnode/README.md"
topio_service_path="./src/xtopcom/xnode/topio_service.sh"


if [ "debug" == $debug_release_mode ]; then
    echo "pack in debug mode"
    tmp_debug_tar_path="topio-$topio_version-debug"
    mkdir -p $tmp_debug_tar_path
    echo_and_run echo "cp $debug_build_topio_path                   $tmp_debug_tar_path" |bash -
    echo_and_run echo "cp $debug_build_libxtopchain_so_path         $tmp_debug_tar_path" |bash -
    echo_and_run echo "cp $topio_install_sh_path                    $tmp_debug_tar_path" |bash -
    #echo_and_run echo "cp $topio_readme_path                        $tmp_debug_tar_path" |bash -
    echo_and_run echo "cp $topio_service_path                        $tmp_debug_tar_path" |bash -
    echo_and_run echo "cp $topio_uninstall_sh_path                  $tmp_debug_tar_path" |bash -
    echo_and_run echo "tar zcvf topio-$topio_version-debug.tar.gz   $tmp_debug_tar_path" |bash -
    /usr/bin/rm -rf $tmp_debug_tar_path
elif [ "release" == $debug_release_mode ]; then
    echo "pack in release mode"
    tmp_release_tar_path="topio-$topio_version-release"
    mkdir -p $tmp_release_tar_path
    echo_and_run echo "cp $release_build_topio_path                 $tmp_release_tar_path" |bash -
    echo_and_run echo "cp $release_build_libxtopchain_so_path       $tmp_release_tar_path" |bash -
    echo_and_run echo "cp $topio_install_sh_path                    $tmp_release_tar_path" |bash -
    #echo_and_run echo "cp $topio_readme_path                        $tmp_release_tar_path" |bash -
    echo_and_run echo "cp $topio_service_path                        $tmp_release_tar_path" |bash -
    echo_and_run echo "cp $topio_uninstall_sh_path                  $tmp_release_tar_path" |bash -
    echo_and_run echo "tar zcvf topio-$topio_version-release.tar.gz   $tmp_release_tar_path" |bash -
    /usr/bin/rm -rf $tmp_release_tar_path
fi

echo ""
echo "available installation package as below:"
ls topio-$topio_version-$debug_release_mode.tar.gz
echo "######################end tar topio-package#############################"
