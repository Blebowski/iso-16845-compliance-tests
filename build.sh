#!/bin/bash

mkdir -p build/Debug
mkdir -p build/Release

# Debug build
cd build/Debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..
make
cd ../..
cd /D
# Release build
cd build/Release
cmake -DCMAKE_BUILD_TYPE=Release ../..
make
cd ../..
