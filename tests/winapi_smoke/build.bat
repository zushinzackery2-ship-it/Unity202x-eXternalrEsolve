@echo off
call "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
msbuild Er2WinApiSmoke.vcxproj /t:Build /p:Configuration=Release;Platform=x64
