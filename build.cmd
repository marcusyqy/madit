@echo off
setlocal enabledelayedexpansion

REM to make sure that we cd into the right directory.
cd /d "%~dp0"

for %%a in (%*) do set "%%a=1"

set CC=cl.exe
REM set COMPILE_FLAGS=/W4 /O2 /EHsc /Zi -external:W0
set COMPILE_FLAGS=/W4 /Od /Ob0 /EHsc /Zi -external:W0
set DEFINE_FLAGS=-DVK_NO_PROTOTYPES -DVK_USE_PLATFORM_WIN32_KHR
set BUILD_DIR=.build

if not exist .build mkdir .build
if not exist .build\SDL3.lib call scripts\install_sdl3.cmd || exit /b 1
if not exist .build\vma.lib  call scripts\build_vma.cmd || exit /b 1
if not exist .build\freetype.lib call scripts\build_freetype.cmd || exit /b 1

REM need to replace this awayyy
call python3 scripts\install_vulkan.py

if "%VULKAN_SDK%"=="" (
  echo Vulkan sdk not installed!
  exit /b 1
)

set LINK_FLAGS=/link SDL3.lib vma.lib freetype.lib %VULKAN_SDK%\Lib\shaderc_shared.lib
set INCLUDE_FLAGS=-external:I..\third_party\SDL\include /I..\src -external:I..\third_party -external:I..\third_party\freetype\include /I..\..\mb
REM -external:I%VULKAN_SDK%\Include

pushd .build
call %CC% %COMPILE_FLAGS% %DEFINE_FLAGS% %INCLUDE_FLAGS% ..\src\build.c /Fe:madit.exe %LINK_FLAGS% || (
  del madit.exe
  exit /b 1
)

popd

REM call python3 scripts\generate_compile_flags.py

cmd /c exit %ERRORLEVEL%

