@echo off
setlocal
cd /d "%~dp0"
if not defined VSCMD_ARG_TGT_ARCH (
  for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
  if not defined VSINSTALL (
    echo Install Visual Studio C++ Desktop workload with Windows SDK first.
    exit /b 1
  )
)
if not defined VSCMD_ARG_TGT_ARCH call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"
if /i not "%VSCMD_ARG_TGT_ARCH%"=="x64" (
  echo Run from an x64 Native Tools Command Prompt.
  exit /b 1
)
if not exist build mkdir build
rc /nologo /Isrc /fobuild\plugin.res src\plugin.rc
if errorlevel 1 exit /b 1
cl /nologo /LD /O2 /std:c++17 /EHsc /W4 /utf-8 /MT /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN /Isdk\include src\plugin.cpp build\plugin.res /Fobuild\plugin.obj /link /OUT:build\channel_tts_win64.dll /IMPLIB:build\channel_tts.lib sapi.lib ole32.lib uuid.lib user32.lib gdi32.lib comctl32.lib
exit /b %errorlevel%
