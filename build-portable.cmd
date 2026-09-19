@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem ================================================================
rem King Panel v1.0.1 portable build
rem Requires: Zig 0.13.0
rem Does NOT require NSIS.
rem
rem Double-click this file to build dist\KingPanel.exe.
rem Pass "nopause" when running in CI/automation.
rem ================================================================

set "PAUSE_AT_END=1"
if /I "%~1"=="nopause" set "PAUSE_AT_END=0"

title King Panel - Portable Build v1.0.1
echo ========================================
echo   King Panel v1.0.1 Portable Build
echo ========================================
echo.

set "ZIG_VERSION="

rem Respect ZIG_EXE only when it points to a real file.
if defined ZIG_EXE if exist "%ZIG_EXE%" goto zig_found
if defined ZIG_EXE (
    echo Warning: ZIG_EXE points to a file that does not exist:
    echo   %ZIG_EXE%
    set "ZIG_EXE="
)

rem First try PATH, then common Windows install locations.
for /f "delims=" %%I in ('where zig.exe 2^>nul') do if not defined ZIG_EXE set "ZIG_EXE=%%~fI"
if not defined ZIG_EXE if exist "%ProgramFiles%\Zig\zig.exe" set "ZIG_EXE=%ProgramFiles%\Zig\zig.exe"
if not defined ZIG_EXE if exist "%LOCALAPPDATA%\Programs\Zig\zig.exe" set "ZIG_EXE=%LOCALAPPDATA%\Programs\Zig\zig.exe"
if not defined ZIG_EXE if exist "%LOCALAPPDATA%\Microsoft\WinGet\Links\zig.exe" set "ZIG_EXE=%LOCALAPPDATA%\Microsoft\WinGet\Links\zig.exe"
if not defined ZIG_EXE if exist "%USERPROFILE%\scoop\shims\zig.exe" set "ZIG_EXE=%USERPROFILE%\scoop\shims\zig.exe"
if not defined ZIG_EXE if exist "%ProgramData%\chocolatey\bin\zig.exe" set "ZIG_EXE=%ProgramData%\chocolatey\bin\zig.exe"
if not defined ZIG_EXE if exist "C:\zig\zig.exe" set "ZIG_EXE=C:\zig\zig.exe"

if not defined ZIG_EXE (
    echo ERROR: Zig was not found.
    echo.
    echo Install Zig 0.13.0 or make zig.exe available on PATH.
    echo Useful checks:
    echo   where zig
    echo   zig version
    echo.
    echo You may also set ZIG_EXE to the full path to zig.exe.
    set "BUILD_RESULT=1"
    goto finish
)

:zig_found
"%ZIG_EXE%" version >nul 2>nul
if errorlevel 1 (
    echo ERROR: Zig could not be started:
    echo   %ZIG_EXE%
    set "BUILD_RESULT=1"
    goto finish
)
for /f "delims=" %%V in ('"%ZIG_EXE%" version 2^>nul') do set "ZIG_VERSION=%%V"
echo Using Zig %ZIG_VERSION%:
echo   %ZIG_EXE%
echo.

if not exist "dist" mkdir "dist"
if errorlevel 1 (
    echo ERROR: Could not create the dist folder.
    set "BUILD_RESULT=1"
    goto finish
)

echo [1/2] Compiling Windows resources...
"%ZIG_EXE%" rc /fo kingpanel.res kingpanel.rc
if errorlevel 1 goto build_failed

echo [2/2] Compiling portable KingPanel.exe...
"%ZIG_EXE%" cc -target x86_64-windows-gnu -Oz -s -fstack-protector-strong -Wall -Wextra -Werror -Wl,--subsystem,windows kingpanel.c kingpanel.res -o dist\KingPanel.exe -luser32 -lshell32 -lgdi32 -ladvapi32 -ldwmapi -lsetupapi
if errorlevel 1 goto build_failed

del /q kingpanel.res >nul 2>nul
set "BUILD_RESULT=0"
echo.
echo SUCCESS: Built portable app:
echo   %CD%\dist\KingPanel.exe
goto finish

:build_failed
del /q kingpanel.res >nul 2>nul
set "BUILD_RESULT=1"
echo.
echo ERROR: King Panel failed to build. The compiler message above is the reason.

:finish
echo.
echo ========================================
if "%BUILD_RESULT%"=="0" (
    echo PORTABLE BUILD COMPLETE
) else (
    echo PORTABLE BUILD FAILED
)
echo ========================================
echo.
if "%PAUSE_AT_END%"=="1" pause
exit /b %BUILD_RESULT%
