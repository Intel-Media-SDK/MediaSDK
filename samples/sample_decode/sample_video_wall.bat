@echo off
::/******************************************************************************\
:: Copyright (c) 2005-2016, Intel Corporation
:: All rights reserved.
::
:: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
::
:: 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
::
:: 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
::
:: 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
::
:: THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
::
:: This sample was distributed or derived from the Intel's Media Samples package.
:: The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
:: or https://software.intel.com/en-us/media-client-solutions-support.
:: \**********************************************************************************/

setlocal enabledelayedexpansion

set VerticalCells=4
set HorizontalCells=4

rem Set bTitle (0 - off | 1 - on,  will provide rendering rate info) 
set bTitle=0

rem Set MediaSDK implementation (-hw - Hardware,  Use platform-specific implementation of Intel® Media SDK) 
rem set MediaSDKImplementation=-hw


rem Set media type (h264 | mpeg2 | vc1) 
if not defined MediaType set MediaType=h264
rem Set monitor (default: 0) 
set nMonitor=0
rem Set frame rate for rendering
set FrameRate=30
rem Set timeout in seconds
set Timeout=15


if "%1"=="" goto :help

if exist %1\ (set Video_dir=%1) else (if exist %1 (set VideoFile=%1) else (goto :help))

set /a nVideoFiles=0
if defined Video_dir for %%a in (%Video_dir%\*) do set VideoFile!nVideoFiles!=%%a& set /a nVideoFiles+=1
if defined Video_dir if "%nVideoFiles%"=="0" echo Error: No video files found& goto :help

set /a nCells=%VerticalCells% * %HorizontalCells% - 1
set /a nVideoFiles1=0
for /l %%a in (0,1,%nCells%) do (
  if defined Video_dir call :get_VideoFile !nVideoFiles1!
  start /b "" "%~dp0sample_decode.exe" %MediaType% -i "!VideoFile!" %MediaSDKImplementation% -d3d -f %FrameRate% -wall %HorizontalCells% %VerticalCells% %%a %nMonitor% %bTitle% %Timeout%
  set /a nVideoFiles1+=1
  if !nVideoFiles1! GEQ %nVideoFiles% set /a nVideoFiles1=0
)
goto :eof

:get_VideoFile
set VideoFile=!VideoFile%1!
goto :eof

:help
echo Intel(R) Media SDK Video Wall Sample Script
echo.
echo Usage: %~nx0 [stream ^| streams folder]
echo.
pause