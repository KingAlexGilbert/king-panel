@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem ================================================================
rem King Panel v1.0.1 installer build
rem Requires: NSIS 3.08+ and an existing portable EXE in dist.
rem Does NOT require Zig and does NOT rebuild KingPanel.exe.
rem
rem Double-click this file to package dist\KingPanel.exe for x64.
rem Double-click build-installer-arm64.cmd for the ARM64 version.
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
set "APP_NAME=KingPanel.exe"
set "BUILD_SCRIPT=build-installer.cmd"
set "SETUP_NAME=KingPanel-Setup-1.0.1.exe"
if "%TARGET_ARCH%"=="arm64" (
    set "APP_NAME=KingPanel-arm64.exe"
    set "BUILD_SCRIPT=build-installer-arm64.cmd"
    set "SETUP_NAME=KingPanel-Setup-1.0.1-arm64.exe"
)
set "APP_EXE=%CD%\dist\%APP_NAME%"

title King Panel - Installer Build v1.0.1
echo ========================================
echo   King Panel v1.0.1 Installer Build [%TARGET_ARCH%]
echo ========================================
echo.

if not exist "%APP_EXE%" (
    echo ERROR: %APP_NAME% was not found in:
    echo   %CD%\dist
    echo.
    echo This installer-only builder intentionally does not require Zig.
    echo Put the matching built or downloaded %APP_NAME% in the dist folder,
    echo then double-click %BUILD_SCRIPT% again.
    set "BUILD_RESULT=1"
    goto finish
)

rem Read the PE machine type so a renamed EXE cannot be packaged for the wrong CPU.
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -NonInteractive -Command ^
    "$ErrorActionPreference = 'Stop'; $r = [IO.BinaryReader]::new([IO.File]::OpenRead($env:APP_EXE)); try { if ($r.ReadUInt16() -ne 0x5A4D) { throw 'Not a Windows EXE.' }; $r.BaseStream.Position = 0x3C; $pe = $r.ReadUInt32(); $r.BaseStream.Position = $pe; if ($r.ReadUInt32() -ne 0x4550) { throw 'Invalid PE header.' }; $expected = if ($env:TARGET_ARCH -eq 'arm64') { 0xAA64 } else { 0x8664 }; if ($r.ReadUInt16() -ne $expected) { throw ('The EXE does not match the requested ' + $env:TARGET_ARCH + ' architecture.') } } finally { $r.Dispose() }"
if errorlevel 1 (
    echo ERROR: Could not validate the %TARGET_ARCH% executable. See the message above.
    set "BUILD_RESULT=1"
    goto finish
)

rem Respect NSIS_EXE only when it points to a real file.
if defined NSIS_EXE if exist "%NSIS_EXE%" goto nsis_found
if defined NSIS_EXE (
    echo Warning: NSIS_EXE points to a file that does not exist:
    echo   %NSIS_EXE%
    set "NSIS_EXE="
)

rem Try PATH, then common NSIS install locations.
for /f "delims=" %%I in ('where makensis.exe 2^>nul') do if not defined NSIS_EXE set "NSIS_EXE=%%~fI"
if not defined NSIS_EXE if exist "%ProgramFiles(x86)%\NSIS\makensis.exe" set "NSIS_EXE=%ProgramFiles(x86)%\NSIS\makensis.exe"
if not defined NSIS_EXE if exist "%ProgramFiles%\NSIS\makensis.exe" set "NSIS_EXE=%ProgramFiles%\NSIS\makensis.exe"
if not defined NSIS_EXE if exist "%LOCALAPPDATA%\Programs\NSIS\makensis.exe" set "NSIS_EXE=%LOCALAPPDATA%\Programs\NSIS\makensis.exe"

if not defined NSIS_EXE (
    echo ERROR: NSIS 3.08 or newer was not found.
    echo.
    echo Install NSIS 3.08 or newer and make makensis.exe available on PATH.
    echo Useful check:
    echo   where makensis
    echo.
    echo You may also set NSIS_EXE to the full path to makensis.exe.
    set "BUILD_RESULT=1"
    goto finish
)

:nsis_found
echo Using NSIS:
echo   %NSIS_EXE%
echo.
echo Packaging existing portable executable:
echo   %APP_EXE%
echo.
echo Building %SETUP_NAME%...
"%NSIS_EXE%" /WX /DTARGET_ARCH=%TARGET_ARCH% setup.nsi
if errorlevel 1 (
    echo.
    echo ERROR: The installer failed to build. The NSIS message above is the reason.
    set "BUILD_RESULT=1"
    goto finish
)

set "BUILD_RESULT=0"
echo.
echo SUCCESS: Built installer:
echo   %CD%\dist\%SETUP_NAME%

:finish
echo.
echo ========================================
if "%BUILD_RESULT%"=="0" (
    echo INSTALLER BUILD COMPLETE
) else (
    echo INSTALLER BUILD FAILED
)
echo ========================================
echo.
if "%PAUSE_AT_END%"=="1" pause
exit /b %BUILD_RESULT%
