@echo off

set C_FLAGS= /std:c++17 /GFyLA /EHsc /Zi /O2 /Oi /guard:cf- /arch:AVX /Qvec-report:1 /MD /sdl- /nologo /D_CRT_SECURE_NO_WARNINGS /DNDEBUG /I..\..\..\folly-json /IC:\Src\sdk\boost_1_67_0 /I..\..\..\double-conversion-3.1.4
set L_FLAGS= /link /OPT:ICF

cl.exe %C_FLAGS% test_fj.cpp %L_FLAGS% ..\..\..\folly-json\bin\folly_json.obj ..\..\..\folly-json\bin\String.obj ..\..\..\double-conversion-3.1.4\msvc\Release\x64\double-conversion.lib