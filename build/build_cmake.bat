@echo off
rem ============================================================
rem  CMake + Ninja build for IPv4OctetSort.
rem
rem  Why this wrapper exists: CMake/Ninja generate object paths that
rem  include the source directory name, and the MinGW toolchain fails
rem  on non-ASCII paths. This script builds through an ASCII junction
rem  in the temp directory, so the project folder may keep its
rem  Chinese name. The artifact is written to the real <project>\dist
rem  folder and runs from there.
rem
rem  The junction is <TEMP>\ipv4src -> <project>; delete it any time.
rem
rem  Keep this file pure ASCII with CRLF line endings (see build.bat
rem  for the reason).
rem ============================================================
setlocal
pushd "%~dp0.."
set "ROOT=%CD%"
set "LINK=%TEMP%\ipv4src"
set "BUILD=%LINK%\build\ninja"

rem The 32-bit compiler needs its own bin directory first on PATH,
rem otherwise CMake's compiler check fails.
if not defined MINGW32_BIN set "MINGW32_BIN=C:\msys64\mingw32\bin"
if not defined MINGW64_BIN set "MINGW64_BIN=C:\msys64\mingw64\bin"
set "PATH=%MINGW32_BIN%;%MINGW64_BIN%;%PATH%"

where cmake >nul 2>nul
if errorlevel 1 (
  echo [ERROR] cmake not found in PATH.
  popd
  exit /b 1
)

if exist "%LINK%\src\main.cpp" goto :have_link
if exist "%LINK%" rmdir "%LINK%"
cmd /c mklink /J "%LINK%" "%ROOT%" >nul
if errorlevel 1 (
  echo [ERROR] could not create the junction %LINK%
  popd
  exit /b 1
)

:have_link
if not exist "%LINK%\src\main.cpp" (
  echo [ERROR] junction %LINK% does not point at the project
  popd
  exit /b 1
)

echo [1/2] configuring ...
cmake -G Ninja -S "%LINK%\build" -B "%BUILD%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto :failed

echo [2/2] building ...
cmake --build "%BUILD%"
if errorlevel 1 goto :failed

echo.
echo Done. Output folder: %ROOT%\dist
popd
exit /b 0

:failed
echo.
echo BUILD FAILED
popd
exit /b 1
