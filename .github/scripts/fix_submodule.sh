#!/bin/bash
source /etc/profile
set -x

# BRANCH=$1

git submodule update --init --recursive

# BASE_DIR=$(pwd)
# echo ${BASE_DIR}
# cd src/xtopcom/xvm && git fetch && git checkout ${BRANCH} && git pull
# cd ${BASE_DIR}
# cd src/xtopcom/xbase && git fetch && git checkout ${BRANCH} && git pull
# cd ${BASE_DIR}

echo "finish submodule"
