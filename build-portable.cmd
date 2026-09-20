@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem ================================================================
rem King Panel v1.0.1 portable build
rem Requires: Zig 0.13.0
rem Does NOT require NSIS.
rem
rem Double-click this file to build dist\KingPanel.exe for x64.
rem Double-click build-portable-arm64.cmd for the ARM64 version.
rem Pass "nopause" when running in CI/automation.
rem ================================================================

set "TARGET_ARCH=x64"
set "PAUSE_AT_END=1"

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="nopause" (
    set "PAUSE_AT_END=0"
) else if /I "%~1"=="x64" (
    set "TARGET_ARCH=x64"
) else if /I "%~1"=="arm64" (
    set "TARGET_ARCH=arm64"
) else (
    echo ERROR: Unknown argument "%~1".
    echo Usage: %~nx0 [x64^|arm64] [nopause]
    exit /b 1
)
shift
goto parse_args

:args_done
set "ZIG_TARGET=x86_64-windows-gnu"
set "APP_NAME=KingPanel.exe"
if "%TARGET_ARCH%"=="arm64" (
    set "ZIG_TARGET=aarch64-windows-gnu"
    set "APP_NAME=KingPanel-arm64.exe"
)
set "RESOURCE_FILE=dist\kingpanel-%TARGET_ARCH%.res"

title King Panel - Portable Build v1.0.1
echo ========================================
echo   King Panel v1.0.1 Portable Build [%TARGET_ARCH%]
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
"%ZIG_EXE%" rc /fo "%RESOURCE_FILE%" kingpanel.rc
if errorlevel 1 goto build_failed

echo [2/2] Compiling portable %APP_NAME%...
"%ZIG_EXE%" cc -target %ZIG_TARGET% -Oz -s -fstack-protector-strong -Wall -Wextra -Werror -Wl,--subsystem,windows kingpanel.c "%RESOURCE_FILE%" -o "dist\%APP_NAME%" -luser32 -lshell32 -lgdi32 -ladvapi32 -ldwmapi -lsetupapi
if errorlevel 1 goto build_failed

del /q "%RESOURCE_FILE%" >nul 2>nul
set "BUILD_RESULT=0"
echo.
echo SUCCESS: Built portable app:
echo   %CD%\dist\%APP_NAME%
goto finish

:build_failed
del /q "%RESOURCE_FILE%" >nul 2>nul
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
