@echo off
set C_FLAGS= /GFyLA /std:c++17 /EHsc /Zi /O2 /Oi /guard:cf- /arch:AVX /Qvec-report:1 /MD /sdl- /W4 /nologo /D_CRT_SECURE_NO_WARNINGS /DNDEBUG
set C_FLAGS= %C_FLAGS% /I ..\..\..\tinyxml2-7.0.1\
set L_FLAGS= /link /OPT:ICF


if '%Platform%' == 'x86' (
cl.exe %C_FLAGS% test_tiny2.cpp %L_FLAGS% ..\..\..\tinyxml2-7.0.1\proj\Release-Lib\tinyxml2.lib
) else (
cl.exe %C_FLAGS% test_tiny2.cpp %L_FLAGS% ..\..\..\tinyxml2-7.0.1\proj\bin\tinyxml2\x64-Release-Lib\tinyxml2.lib
)