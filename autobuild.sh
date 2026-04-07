#!/bin/bash

set -e

mkdir -p build

rm -rf $(pwd)/build/*

cd src
protoc --cpp_out=. rpcheader.proto

cd ..
mv src/rpcheader.pb.h src/include/

cd $(pwd)/build &&
    cmake .. &&
    make
