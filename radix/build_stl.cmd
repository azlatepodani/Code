@echo off
set C_FLAGS= /GmFyLA /EHsc /Zi /O2 /Oi /guard:cf- /Qvec-report:1 /arch:AVX /sdl- /nologo /D_CRT_SECURE_NO_WARNINGS
rem set C_FLAGS= %C_FLAGS% /analyze 
set L_FLAGS= /link /OPT:ICF

cl.exe %C_FLAGS% /MD test_stl.cpp %L_FLAGS% 

set BENCH_INCLUDE=/I..\..\benchmark\include
set BENCH_LINK=/LIBPATH:..\..\benchmark\lib\x86 
cl.exe %C_FLAGS% %BENCH_INCLUDE% bench_stl.cpp %L_FLAGS% %BENCH_LINK%