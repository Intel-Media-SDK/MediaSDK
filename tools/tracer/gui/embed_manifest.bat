@echo off

if exist "%SystemDrive%\Program Files\Microsoft SDKs\Windows\v7.1\bin\mt.exe" (
set mt_path="%SystemDrive%\Program Files\Microsoft SDKs\Windows\v7.1\bin\mt.exe"
goto :continue
)
if exist "%SystemDrive%\Program Files\Microsoft SDKs\Windows\v7.0\bin\mt.exe" (
set mt_path="%SystemDrive%\Program Files\Microsoft SDKs\Windows\v7.0\bin\mt.exe"
goto :continue
)
if exist "%SystemDrive%\Program Files\Microsoft SDKs\Windows\v6.1\bin\mt.exe" (
set mt_path="%SystemDrive%\Program Files\Microsoft SDKs\Windows\v6.1\bin\mt.exe"
goto :continue
)
if exist "%SystemDrive%\Program Files\Microsoft SDKs\Windows\v6.0\bin\mt.exe" (
set mt_path="%SystemDrive%\Program Files\Microsoft SDKs\Windows\v6.0\bin\mt.exe"
)

:continue

copy %2tracer.exe.manifest %3
echo %2tracer.exe.manifest %3

cd %1
%mt_path%   -manifest "tracer.exe.manifest" -outputresource:"tracer.exe;#1") 
