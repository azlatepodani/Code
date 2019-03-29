@echo off

set C_FLAGS= /GFyLA /EHsc /Zi /O2 /Oi /guard:cf- /arch:AVX /Qvec-report:1 /MD /sdl- /nologo /D_CRT_SECURE_NO_WARNINGS /DNDEBUG /I..\..\..\chrome-json
set L_FLAGS= /link /OPT:ICF

cl.exe %C_FLAGS% test_cj.cpp %L_FLAGS% ..\..\..\chrome-json\bin\chrome_json.obj ..\..\..\chrome-json\bin\dtoa.obj ..\..\..\chrome-json\bin\g_fmt.obj ..\..\..\chrome-json\bin\icu_utf.obj