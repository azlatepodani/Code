#!/bin/bash

C_FLAGS="-std=c++17 -O2 -I../../../folly-json -I/mnt/c/Src/sdk/boost_1_67_0 -I../../../double-conversion-3.1.4"

clang++ $C_FLAGS test_fj.cpp -o test_fj.out ../../../folly-json/bin/folly_json.o ../../../folly-json/bin/String.o ../../../double-conversion-3.1.4/libdouble-conversion.a