@echo off
REM Compile-only harness for the offline dump layer. The existing tests/unit
REM harness is /std:c++17 and never touches src/dumpsdk/**, so this is the only
REM build that actually type-checks the offline collector and its writers.
call "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul
cd /d "%~dp0"
if not exist obj mkdir obj
cl /nologo /std:c++20 /EHsc /c /W3 /I "..\..\include" /I "..\.." ^
 "..\..\src\dumpsdk\offline\BinaryStream.cpp" ^
 "..\..\src\dumpsdk\offline\Il2CppStructs.cpp" ^
 "..\..\src\dumpsdk\offline\Il2CppMetadataStructs.cpp" ^
 "..\..\src\dumpsdk\offline\Metadata.cpp" ^
 "..\..\src\dumpsdk\offline\MetadataHeader.cpp" ^
 "..\..\src\dumpsdk\offline\MetadataTables.cpp" ^
 "..\..\src\dumpsdk\offline\MetadataTableReader.cpp" ^
 "..\..\src\dumpsdk\offline\MetadataTypes.cpp" ^
 "..\..\src\dumpsdk\offline\MetadataMembers.cpp" ^
 "..\..\src\dumpsdk\offline\MetadataExtras.cpp" ^
 "..\..\src\dumpsdk\offline\MetadataUsage.cpp" ^
 "..\..\src\dumpsdk\offline\PeImage.cpp" ^
 "..\..\src\dumpsdk\offline\PeImageAccess.cpp" ^
 "..\..\src\dumpsdk\offline\SafeHostMemory.cpp" ^
 "..\..\src\dumpsdk\offline\RegistrationSearch.cpp" ^
 "..\..\src\dumpsdk\offline\RegistrationSearchLegacy.cpp" ^
 "..\..\src\dumpsdk\offline\RegistrationSearchModern.cpp" ^
 "..\..\src\dumpsdk\offline\RegistrationSearchPatterns.cpp" ^
 "..\..\src\dumpsdk\offline\RegistrationSearchValidation.cpp" ^
 "..\..\src\dumpsdk\offline\RegistrationSearchScoring.cpp" ^
 "..\..\src\dumpsdk\offline\RegistrationSearchInit.cpp" ^
 "..\..\src\dumpsdk\offline\OfflineRuntimeContext.cpp" ^
 "..\..\src\dumpsdk\offline\OfflineRuntimeTypes.cpp" ^
 "..\..\src\dumpsdk\offline\OfflineRuntimeMethods.cpp" ^
 "..\..\src\dumpsdk\offline\OfflineRuntimeGenerics.cpp" ^
 "..\..\src\dumpsdk\offline\TypeNameResolverCore.cpp" ^
 "..\..\src\dumpsdk\offline\TypeNameResolverNames.cpp" ^
 "..\..\src\dumpsdk\offline\TypeNameResolverGenerics.cpp" ^
 "..\..\src\dumpsdk\offline\DefaultValueDecoder.cpp" ^
 "..\..\src\dumpsdk\offline\CustomAttributeReader.cpp" ^
 "..\..\src\dumpsdk\offline\CustomAttributeReaderPost29.cpp" ^
 "..\..\src\dumpsdk\offline\CollectFlags.cpp" ^
 "..\..\src\dumpsdk\offline\CollectMembers.cpp" ^
 "..\..\src\dumpsdk\offline\CollectMethods.cpp" ^
 "..\..\src\dumpsdk\offline\CollectStrings.cpp" ^
 "..\..\src\dumpsdk\offline\OfflineCollector.cpp" ^
 "..\..\src\dumpsdk\offline\OfflineDumper.cpp" ^
 "..\..\src\dumpsdk\writers\sidecar_writer.cpp" ^
 "..\..\src\dumpsdk\writers\sidecar_dump_cs.cpp" ^
 "..\..\src\dumpsdk\writers\sidecar_header.cpp" ^
 "..\..\src\dumpsdk\writers\sidecar_json.cpp" ^
 "..\..\src\dumpsdk\writers\sidecar_text.cpp" ^
 "..\..\src\dumpsdk\writers\sidecar_signatures.cpp" ^
 /Fo:obj\ > build_log.txt 2>&1
echo EXITCODE=%ERRORLEVEL% >> build_log.txt
