#!/bin/sh

solc --abi eth2client.sol -o build
cd build
abigen --abi eth2client.abi --pkg eth2client --type Eth2Client --out eth2client.go
cd ..
