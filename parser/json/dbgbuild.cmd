@echo off
set C_FLAGS= /EHsc /Zi /guard:cf- /sdl- /W4 /nologo /D_CRT_SECURE_NO_WARNINGS /DDEBUG
rem set C_FLAGS= %C_FLAGS% /analyze 
set L_FLAGS=

cl.exe /c %C_FLAGS% azp_json.cpp
cl.exe /c %C_FLAGS% azp_json_api.cpp
cl.exe %C_FLAGS% test_azpj2.cpp %L_FLAGS% azp_json.obj azp_json_api.obj
