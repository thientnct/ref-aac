@echo off
REM ---------------------------------------------------------------------------
REM clean.bat - remove build artifacts generated during mp4auenc build sessions.
REM
REM What it does:
REM   - Deletes auxiliary build dirs: build-prof\, build-pgo\  (safe, ours)
REM   - Inside build\: deletes compiler/CMake artifacts (*.o *.a *.exe *.obj
REM     *.gcda *.gcno gmon.out CMakeFiles\ cmake_install.cmake CMakeCache.txt
REM     build.ninja .ninja_*)
REM   - KEEPS media/input files in build\ (*.wav *.mp4 *.aac *.raw ...)
REM ---------------------------------------------------------------------------
setlocal
set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"

echo === Removing auxiliary build dirs (build-prof, build-pgo) ===
for %%D in (build-prof build-pgo) do (
  if exist "%ROOT%\%%D" (
    echo   - %%D\
    rmdir /s /q "%ROOT%\%%D"
  )
)

if not exist "%ROOT%\build" goto :done

echo === Cleaning artifacts inside build\ (media files are kept) ===
pushd "%ROOT%\build"

del /q *.o *.a *.obj *.exe *.gcda *.gcno *.gch gmon.out gprof.txt 2>nul
for /d %%X in (CMakeFiles .ninja_deps .ninja_log) do rmdir /s /q "%%X" 2>nul
del /q CMakeCache.txt cmake_install.cmake build.ninja .ninja_deps .ninja_log 2>nul

REM remove per-target CMakeFiles dirs nested deeper
for /d %%X in (*) do (
  if exist "%%X\CMakeFiles" rmdir /s /q "%%X\CMakeFiles" 2>nul
)

popd

:done
echo.
echo Clean done. Run build_mp4AudVm_Rewrite_windows.bat to rebuild.
endlocal
