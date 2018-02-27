@echo off
set C_FLAGS= /GmFyLA /EHsc /Zi /O2 /Oi /guard:cf- /Qvec-report:1 /MD /arch:AVX /sdl- /nologo /D_CRT_SECURE_NO_WARNINGS /DNDEBUG
rem set C_FLAGS= %C_FLAGS% /analyze 
set C_FLAGS= %C_FLAGS%  /DUNICODE /D_UNICODE /I"R:\Src\sdk\VS 2012\jsoncpp\1.8.4\include"
set L_FLAGS= /link /OPT:ICF /LIBPATH:"R:\Src\sdk\VS 2012\jsoncpp\1.8.4\lib_x86\v140_xp\Release Lib"

cl.exe %C_FLAGS% test_jsoncpp.cpp %L_FLAGS% 
