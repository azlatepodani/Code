@echo off
set C_FLAGS= /GFyLA /std:c++17 /EHsc /Zi /O2 /Oi /guard:cf- /arch:AVX /Qvec-report:1 /MD /sdl- /W4 /nologo /D_CRT_SECURE_NO_WARNINGS /DNDEBUG
set C_FLAGS= %C_FLAGS% /I C:\Src\tinyxml_2_6_2
set L_FLAGS= /link /OPT:ICF

cl.exe %C_FLAGS% test_tiny.cpp %L_FLAGS% C:\Src\tinyxml_2_6_2\tinyxml\Releasetinyxml\tinyxml.lib
