@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem ================================================================
rem King Panel v1.0.1 installer build
rem Requires: NSIS 3 and an existing dist\KingPanel.exe.
rem Does NOT require Zig and does NOT rebuild KingPanel.exe.
rem
rem Double-click this file to package dist\KingPanel.exe into the installer.
rem Pass "nopause" when running in CI/automation.
rem ================================================================

set "PAUSE_AT_END=1"
if /I "%~1"=="nopause" set "PAUSE_AT_END=0"

title King Panel - Installer Build v1.0.1
echo ========================================
echo   King Panel v1.0.1 Installer Build
echo ========================================
echo.

if not exist "dist\KingPanel.exe" (
    echo ERROR: KingPanel.exe was not found in:
    echo   %CD%\dist
    echo.
    echo This installer-only builder intentionally does not require Zig.
    echo Put a previously built or downloaded KingPanel.exe in the dist folder,
    echo then run build-installer.cmd again.
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
    echo ERROR: NSIS 3 was not found.
    echo.
    echo Install NSIS 3 or make makensis.exe available on PATH.
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
echo   %CD%\dist\KingPanel.exe
echo.
echo Building KingPanel-Setup-1.0.1.exe...
"%NSIS_EXE%" /WX setup.nsi
if errorlevel 1 (
    echo.
    echo ERROR: The installer failed to build. The NSIS message above is the reason.
    set "BUILD_RESULT=1"
    goto finish
)

set "BUILD_RESULT=0"
echo.
echo SUCCESS: Built installer:
echo   %CD%\dist\KingPanel-Setup-1.0.1.exe

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
