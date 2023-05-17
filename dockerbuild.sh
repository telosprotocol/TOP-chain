#!/bin/bash 
set -e
./build.sh release metrics
rm -f ./docker_build/topio
cp ./cbuild_release/bin/Linux/topio ./scripts/docker_build/topio
chmod +x ./scripts/docker_build/topio
docker build -t topio:${1:-"latest"} -f Dockerfile scripts/docker_build/.