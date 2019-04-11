#!/bin/bash

#g++-8 -std=c++17 -O2 -Wno-logical-op-parentheses test_azpx.cpp azp_xml.cpp azp_xml_api.cpp -o test_azpx.out
clang++-7 -std=c++17 -O2 -Wno-logical-op-parentheses test_azpx.cpp azp_xml.cpp azp_xml_api.cpp -o test_azpx.out
