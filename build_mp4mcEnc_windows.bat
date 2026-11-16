@echo off
REM ---------------------------------------------------------------------------
REM build_mp4mcEnc_windows.bat
REM Build mp4mcenc (MPEG-4 AAC Main profile encoder) with
REM MinGW64 + CMake + Ninja.
REM Usage: build_mp4mcEnc_windows.bat [Debug|Release]
REM   Default: Release
REM Output: build\mp4mcEnc\mp4mcenc.exe  (copied to build\mp4mcenc.exe)
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

echo === Building mp4mcenc ===
"%CMAKE%" --build "%BUILD%" --target mp4mcenc || exit /b 1

echo === Copying mp4mcenc.exe to %BUILD%\mp4mcenc.exe ===
copy /y "%BUILD%\mp4mcEnc\mp4mcenc.exe" "%BUILD%\mp4mcenc.exe" >nul
if errorlevel 1 (
  echo ERROR: failed to copy mp4mcenc.exe
  exit /b 1
)

echo.
echo Build OK: %BUILD%\mp4mcEnc\mp4mcenc.exe
echo Build OK: %BUILD%\mp4mcenc.exe
echo Example: mp4mcenc.exe song1.wav -r 128000 -o song1.aac
endlocal
