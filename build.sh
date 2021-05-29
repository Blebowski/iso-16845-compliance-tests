#!/bin/bash

mkdir -p build
mkdir -p build/Debug
mkdir -p build/Release

cd build

# Debug build
cd Debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..
make -j `nproc`
cd ..

# Release build
cd Release
cmake -DCMAKE_BUILD_TYPE=Release ../..
make -j `nproc`
cd ..
