@echo off
call "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul
cd /d "%~dp0"
if not exist obj mkdir obj
if not exist bin mkdir bin
cl /nologo /std:c++20 /EHsc /O2 /MT /W4 /I "..\..\include" ^
 "ProgressSmoke.cpp" ^
 /Fe:bin\ProgressSmoke.exe /Fo:obj\ > build_log.txt 2>&1
set BUILD_EXIT=%ERRORLEVEL%
echo BUILD_EXIT=%BUILD_EXIT% >> build_log.txt
if not %BUILD_EXIT%==0 exit /b %BUILD_EXIT%
bin\ProgressSmoke.exe > test_log.txt 2>&1
set TEST_EXIT=%ERRORLEVEL%
echo TEST_EXIT=%TEST_EXIT% >> test_log.txt
exit /b %TEST_EXIT%
