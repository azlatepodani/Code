@echo off
set C_FLAGS= /GmFyLA /EHsc /Zi /O2 /Oi /guard:cf- /arch:AVX /Qvec-report:1 /sdl- /W4 /nologo /D_CRT_SECURE_NO_WARNINGS
rem set C_FLAGS= %C_FLAGS% /analyze 
set L_FLAGS= /link /OPT:ICF

cl.exe %C_FLAGS% -I".\..\..\radix" -I. tests\test_ska.cpp %L_FLAGS% 