@echo off
call "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul
cd /d "%~dp0"
if not exist obj mkdir obj
if not exist bin mkdir bin
cl /nologo /std:c++20 /EHsc /O2 /MT /W3 /I "..\..\include" /I "..\.." ^
 "GlobalStringXrefSmoke.cpp" ^
 "..\..\src\dumpsdk\offline\BinaryStream.cpp" ^
 "..\..\src\dumpsdk\offline\PeImage.cpp" ^
 "..\..\src\dumpsdk\offline\SafeHostMemory.cpp" ^
 "..\..\src\dumpsdk\xrefs\GlobalStringCatalog.cpp" ^
 "..\..\src\dumpsdk\xrefs\GlobalStringDecoder.cpp" ^
 "..\..\src\dumpsdk\xrefs\GlobalStringXrefDocumentWriter.cpp" ^
 "..\..\src\dumpsdk\xrefs\GlobalStringXrefExporter.cpp" ^
 "..\..\src\dumpsdk\xrefs\X64InstructionDecoder.cpp" ^
 "..\..\src\dumpsdk\xrefs\X64ReferenceScanner.cpp" ^
 /Fe:bin\GlobalStringXrefSmoke.exe /Fo:obj\ /link Psapi.lib > build_log.txt 2>&1
set BUILD_EXIT=%ERRORLEVEL%
echo BUILD_EXIT=%BUILD_EXIT% >> build_log.txt
if not %BUILD_EXIT%==0 exit /b %BUILD_EXIT%
bin\GlobalStringXrefSmoke.exe > test_log.txt 2>&1
set TEST_EXIT=%ERRORLEVEL%
echo TEST_EXIT=%TEST_EXIT% >> test_log.txt
exit /b %TEST_EXIT%
