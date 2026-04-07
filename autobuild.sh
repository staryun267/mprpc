#!/bin/bash

set -e

mkdir -p build

rm -rf $(pwd)/build/*

cd $(pwd)/build &&
    cmake .. &&
    make
