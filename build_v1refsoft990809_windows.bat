@echo off
REM ---------------------------------------------------------------------------
REM build_v1refsoft990809_windows.bat
REM Build mp4auenc + mp4audec (MPEG-4 Audio VM reference, TF/AAC codec family)
REM with MinGW64 + CMake + Ninja.
REM Usage: build_v1refsoft990809_windows.bat [Debug|Release]
REM   Default: Release
REM Output: build\v1refsoft990809\mp4auenc.exe, mp4audec.exe
REM          (copied to build\mp4auenc.exe, build\mp4audec.exe)
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

echo === Building mp4auenc mp4audec ===
"%CMAKE%" --build "%BUILD%" --target v1_mp4auenc v1_mp4audec || exit /b 1

echo === Copying executables to %BUILD% ===
copy /y "%BUILD%\v1refsoft990809\mp4auenc.exe" "%BUILD%\mp4auenc.exe" >nul
if errorlevel 1 (
  echo ERROR: failed to copy mp4auenc.exe
  exit /b 1
)
copy /y "%BUILD%\v1refsoft990809\mp4audec.exe" "%BUILD%\mp4audec.exe" >nul
if errorlevel 1 (
  echo ERROR: failed to copy mp4audec.exe
  exit /b 1
)

echo.
echo Build OK: %BUILD%\v1refsoft990809\mp4auenc.exe
echo Build OK: %BUILD%\v1refsoft990809\mp4audec.exe
echo Build OK: %BUILD%\mp4auenc.exe
echo Build OK: %BUILD%\mp4audec.exe
endlocal
