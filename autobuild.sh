#!/bin/bash

set -e

mkdir -p build

rm -rf $(pwd)/build/*

cd src

protoc --cpp_out=. rpcheader.proto

mv rpcheader.pb.h include/

cd ..

cd build &&
    cmake .. &&
    make
