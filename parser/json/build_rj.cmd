@echo off
set C_FLAGS= /GmFyLA /EHsc /Zi /O2 /Oi /guard:cf- /Qvec-report:1 /arch:AVX /sdl- /nologo /D_CRT_SECURE_NO_WARNINGS /DNDEBUG
rem set C_FLAGS= %C_FLAGS% /analyze 
set C_FLAGS= %C_FLAGS%  /DUNICODE /D_UNICODE /I"c:\Src\sdk\VS 2012\rapidjson\1.0.2\include"
set L_FLAGS= /link /OPT:ICF 

cl.exe %C_FLAGS% test_rj.cpp %L_FLAGS% 
