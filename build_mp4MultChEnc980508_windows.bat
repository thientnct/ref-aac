@echo off
REM ---------------------------------------------------------------------------
REM build_mp4MultChEnc980508_windows.bat
REM Build mp4multchenc (MPEG-4 multichannel AAC encoder) with
REM MinGW64 + CMake + Ninja.
REM Usage: build_mp4MultChEnc980508_windows.bat [Debug|Release]
REM   Default: Release
REM Output: build\mp4MultChEnc980508\mp4multchenc.exe
REM          (copied to build\mp4multchenc.exe)
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

echo === Building mp4multchenc ===
"%CMAKE%" --build "%BUILD%" --target mp4multchenc || exit /b 1

echo === Copying mp4multchenc.exe to %BUILD%\mp4multchenc.exe ===
copy /y "%BUILD%\mp4MultChEnc980508\mp4multchenc.exe" "%BUILD%\mp4multchenc.exe" >nul
if errorlevel 1 (
  echo ERROR: failed to copy mp4multchenc.exe
  exit /b 1
)

echo.
echo Build OK: %BUILD%\mp4MultChEnc980508\mp4multchenc.exe
echo Build OK: %BUILD%\mp4multchenc.exe
endlocal
