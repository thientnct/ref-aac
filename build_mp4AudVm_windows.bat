@echo off
REM ---------------------------------------------------------------------------
REM build_mp4AudVm_windows.bat
REM Build mp4auenc (MPEG-4 Audio VM encoder: PAR/LPC/TF/HVXC/SSC) with
REM MinGW64 + CMake + Ninja.
REM Usage: build_mp4AudVm_windows.bat [Debug|Release]
REM   Default: Release
REM Output: build\mp4AudVm\mp4auenc.exe  (copied to build\mp4auenc_vm.exe)
REM ---------------------------------------------------------------------------
setlocal

set "MINGW=C:\Users\Admin\Downloads\C2412L\mingw64"
set "CMAKE=%MINGW%\bin\cmake.exe"
set "SRC=%~dp0"
set "SRC=%SRC:~0,-1%"
set "BUILD=%SRC%\build"
set "BUILD_TYPE=%~1"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Release"

if not exist "%CMAKE%" (
  echo ERROR: cmake not found at "%CMAKE%"
  exit /b 1
)

echo === Configuring %BUILD_TYPE% build ===
"%CMAKE%" -S "%SRC%" -B "%BUILD%" -G Ninja -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_C_COMPILER="%MINGW%\bin\gcc.exe" -DCMAKE_AR="%MINGW%\bin\ar.exe" -DCMAKE_RANLIB="%MINGW%\bin\ranlib.exe" -DCMAKE_MAKE_PROGRAM="%MINGW%\bin\ninja.exe" || exit /b 1

echo === Building mp4auenc_vm ===
"%CMAKE%" --build "%BUILD%" --target mp4auenc_vm || exit /b 1

echo === Copying mp4auenc.exe to %BUILD%\mp4auenc_vm.exe ===
copy /y "%BUILD%\mp4AudVm\mp4auenc.exe" "%BUILD%\mp4auenc_vm.exe" >nul
if errorlevel 1 (
  echo ERROR: failed to copy mp4auenc.exe
  exit /b 1
)

echo.
echo Build OK: %BUILD%\mp4auenc_vm.exe
echo Example: mp4auenc_vm.exe song1.wav -m tf -r 128000 -c "-qc_aac" -o song1.mp4
endlocal
