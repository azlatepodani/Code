#!/bin/bash

clang++ -std=c++14 -O2 -Wno-logical-op-parentheses test_azpj2.cpp azp_json.cpp azp_json_api.cpp -o test_azpj2.out
