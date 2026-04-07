#!/bin/bash

set -e

mkdir -p build

rm -rf $(pwd)/build/*

protoc --cpp_out=$(pwd)/src/ $(pwd)/src/rpcheader.proto

mv $(pwd)/src/rpcheader.pb.h $(pwd)/src/include/

cd $(pwd)/build &&
    cmake .. &&
    make
