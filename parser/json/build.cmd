@echo off
set C_FLAGS= /GmFyLA /EHsc /Zi /O2 /Oi /guard:cf- /Qvec-report:1 /arch:AVX /sdl- /W4 /nologo /D_CRT_SECURE_NO_WARNINGS
rem set C_FLAGS= %C_FLAGS% /analyze 
set L_FLAGS= /link /OPT:ICF

cl.exe /c %C_FLAGS% azp_json.cpp
cl.exe /c %C_FLAGS% azp_json_api.cpp
cl.exe %C_FLAGS% test_azpj2.cpp %L_FLAGS% azp_json.obj azp_json_api.obj
