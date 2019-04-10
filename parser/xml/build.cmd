@echo off
set C_FLAGS= /GFyLA /std:c++17 /EHsc /Zi /O2 /Oi /guard:cf- /arch:AVX /Qvec-report:1 /MD /sdl- /W4 /nologo /D_CRT_SECURE_NO_WARNINGS /DNDEBUG
rem set C_FLAGS= %C_FLAGS% /analyze 
set L_FLAGS= /link /OPT:ICF

cl.exe /c %C_FLAGS% azp_xml.cpp
cl.exe /c %C_FLAGS% azp_xml_api.cpp
cl.exe %C_FLAGS% test_azpx.cpp %L_FLAGS% azp_xml.obj azp_xml_api.obj
