@echo off
rem ============================================================
rem  Build and run the core unit tests (console, no UI needed).
rem  Spec ref: tests/test_core.cpp - AC-01..AC-19 algorithm part.
rem  Keep this file pure ASCII (see note in build.bat).
rem ============================================================
setlocal
rem Relative paths only - see the note in build.bat.
pushd "%~dp0.."

where g++ >nul 2>nul
if errorlevel 1 (
  echo [ERROR] g++ not found in PATH.
  popd
  exit /b 1
)

if not exist dist mkdir dist

g++ -std=c++17 -O2 -DUNICODE -D_UNICODE -finput-charset=UTF-8 -fexec-charset=UTF-8 ^
  -Wall -Wextra -Isrc -static -s -o dist\test_core.exe ^
  tests\test_core.cpp ^
  src\core\tokenizer.cpp src\core\parser.cpp src\core\comparator.cpp src\core\sorter.cpp ^
  src\core\segment.cpp src\core\dedupe.cpp src\core\slot_filler.cpp src\core\pipeline.cpp ^
  src\core\lng_file.cpp
if errorlevel 1 (
  echo [ERROR] test build failed.
  popd
  exit /b 1
)

echo Running core unit tests (including language-pack checks) ...
dist\test_core.exe languages
if errorlevel 1 (
  echo [ERROR] tests failed.
  popd
  exit /b 1
)
popd
exit /b %errorlevel%
