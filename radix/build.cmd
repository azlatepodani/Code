@echo off
set C_FLAGS= /GmFyLA /EHsc /Zi /O2 /Oi /guard:cf- /arch:AVX /sdl- /W4 /nologo /D_CRT_SECURE_NO_WARNINGS
rem set C_FLAGS= %C_FLAGS% /analyze /Qvec-report:1
set L_FLAGS= /link /OPT:ICF

cl.exe %C_FLAGS% /MD test.cpp %L_FLAGS% 

set BENCH_INCLUDE=/I..\..\benchmark\include
set BENCH_LINK=/LIBPATH:..\..\benchmark\lib\x86 
cl.exe %C_FLAGS% %BENCH_INCLUDE% bench.cpp %L_FLAGS% %BENCH_LINK%