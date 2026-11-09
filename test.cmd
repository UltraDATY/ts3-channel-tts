@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "TTS_VS=%%i"
if not defined TTS_VS exit /b 1
call "%TTS_VS%\VC\Auxiliary\Build\vcvars64.bat"
call build.cmd
if errorlevel 1 exit /b 1
copy /y build\channel_tts_win64.dll channel_tts_win64.dll >nul
for %%t in (smoke filter ignored_users words_dialog language workflow) do (
  cl /nologo /std:c++17 /utf-8 /EHsc /W4 /MT /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN /Isdk\include tests\%%t.cpp build\plugin.res /Fobuild\%%t.obj /Febuild\%%t.exe /link sapi.lib ole32.lib uuid.lib
  if errorlevel 1 exit /b 1
  build\%%t.exe > build\%%t.log
  if not "!errorlevel!"=="0" (
    type build\%%t.log
    exit /b 1
  )
  type build\%%t.log
)
