@echo off
setlocal EnableExtensions EnableDelayedExpansion

echo [Update] Start
echo [Update] ScriptDir=%~dp0
echo [Update] CallerCwd=%CD%

pushd "%~dp0" >nul
if errorlevel 1 (
    echo [Update] ERROR: pushd failed
    exit /b 1
)

echo [Update] WorkDir=%CD%
set "workdir=%CD%"

set "out=AccessorContainer.hpp"
set "tmp=%TEMP%\AccessorContainer_%RANDOM%%RANDOM%.lst"

echo [Update] OutFile=%out%
echo [Update] TempList=%tmp%

if exist "%tmp%" (
    echo [Update] Remove old temp list
    del /f /q "%tmp%" >nul 2>nul
)

set "root=%CD%\"

echo [Update] ScanRoot=%root%
echo [Update] Scan *.hpp

for /r "%CD%" %%F in (*.hpp) do (
    if /I not "%%~nxF"=="AccessorContainer.hpp" (
        set "rel=%%F"
        set "rel=!rel:%root%=!"
        set "rel=!rel:\=/!"
        echo !rel!>>"%tmp%"
        echo [Update] + !rel!
    )
)

if not exist "%tmp%" (
    echo [Update] ERROR: temp list not created
    popd >nul
    exit /b 1
)

for /f %%A in ('find /c /v "" ^< "%tmp%"') do set "count=%%A"
echo [Update] Found !count! header(s)

echo [Update] Generate %out%
echo #pragma once>"%out%"
if errorlevel 1 goto :fail
echo.>>"%out%"

echo [Update] Write includes

for /f "usebackq delims=" %%L in (`sort "%tmp%"`) do (
    echo #include "%%L">>"%out%"
    echo [Update]   #include "%%L"
)

echo [Update] OutputFile=%workdir%\%out%

echo [Update] Cleanup temp list
del /f /q "%tmp%" >nul 2>nul

popd >nul
echo [Update] Done
exit /b 0

:fail
echo [Update] ERROR: failed to write %out%
if exist "%tmp%" del /f /q "%tmp%" >nul 2>nul
popd >nul
exit /b 1
