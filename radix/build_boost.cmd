@echo off
set C_FLAGS= /GmFyLA /EHsc /Zi /O2 /Oi /guard:cf- /arch:AVX /MD /sdl- /nologo /D_CRT_SECURE_NO_WARNINGS
rem set C_FLAGS= %C_FLAGS% /analyze 
set L_FLAGS= /link /OPT:ICF

cl.exe %C_FLAGS%  test_boost.cpp /I"R:\Src\sdk\VS 2012\boost\1.65.1\include" %L_FLAGS% 
