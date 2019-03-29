#!/bin/bash

clang++ -std=c++14 -O2 test_cj.cpp -I/mnt/c/Src/chrome-json -o test_cj.out /mnt/c/Src/chrome-json/bin/chrome_json.o \
	/mnt/c/Src/chrome-json/bin/dtoa.o /mnt/c/Src/chrome-json/bin/g_fmt.o /mnt/c/Src/chrome-json/bin/icu_utf.o
