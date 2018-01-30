@echo off
set C_FLAGS= /GmFyLA /EHsc /Zi /O2 /Oi /guard:cf- /Qvec-report:1 /arch:AVX /sdl- /nologo /D_CRT_SECURE_NO_WARNINGS
rem set C_FLAGS= %C_FLAGS% /analyze 
set C_FLAGS= %C_FLAGS%  /DUNICODE /D_UNICODE /I"C:\Src\sdk\VS 2012\jsoncpp\1.8.3\include"
set L_FLAGS= /link /OPT:ICF /LIBPATH:"C:\Src\sdk\VS 2012\jsoncpp\1.8.3\lib_x64\v140_xp\Release Static Lib"

cl.exe %C_FLAGS% test_jsoncpp.cpp %L_FLAGS% 
