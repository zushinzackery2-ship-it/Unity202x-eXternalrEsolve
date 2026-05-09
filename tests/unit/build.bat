@echo off
call "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64
cd /d "%~dp0"
if not exist "..\..\bin" mkdir "..\..\bin"
cl /nologo /std:c++17 /EHsc /O2 /W4 /I "..\..\include" /I "E:\UnityEx\endfield\re\Unity202x-eXternalrEsolve" /Fe:..\..\bin\unit_test.exe main.cpp
