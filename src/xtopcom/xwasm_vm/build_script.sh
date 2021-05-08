#!/bin/bash

export CARGO_TARGET_DIR="$2"
cargo build --manifest-path "$1"