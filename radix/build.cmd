@echo off
set C_FLAGS= /GmFyLA /EHsc /Zi /O2 /Oi /guard:cf- /arch:AVX /sdl- /W4 /MD /nologo /D_CRT_SECURE_NO_WARNINGS
rem set C_FLAGS= %C_FLAGS% /analyze /Qvec-report:1
set L_FLAGS= /link /OPT:ICF

cl.exe %C_FLAGS% test.cpp %L_FLAGS% 
