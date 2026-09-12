@echo off
setlocal EnableExtensions
cd /d "%~dp0"
if /I "%~1"=="installer" goto installer
if not "%~1"=="" if /I not "%~1"=="app" goto usage
if defined ZIG_EXE goto foundzig
for /f "delims=" %%I in ('where zig.exe 2^>nul') do if not defined ZIG_EXE set "ZIG_EXE=%%~fI"
if not defined ZIG_EXE if exist "%ProgramFiles%\Zig\zig.exe" set "ZIG_EXE=%ProgramFiles%\Zig\zig.exe"
if not defined ZIG_EXE if exist "%LOCALAPPDATA%\Programs\Zig\zig.exe" set "ZIG_EXE=%LOCALAPPDATA%\Programs\Zig\zig.exe"
if not defined ZIG_EXE if exist "%LOCALAPPDATA%\KingPanel\BuildTools\zig-windows-x86_64-0.13.0\zig.exe" set "ZIG_EXE=%LOCALAPPDATA%\KingPanel\BuildTools\zig-windows-x86_64-0.13.0\zig.exe"
if not defined ZIG_EXE if exist "C:\zig\zig.exe" set "ZIG_EXE=C:\zig\zig.exe"
if not defined ZIG_EXE (
 echo Zig was not found. Install Zig 0.13.0 outside this project, add it to PATH,
 echo or set ZIG_EXE to the full path to zig.exe. See README.md.
 exit /b 1
)
:foundzig
"%ZIG_EXE%" rc /fo kingpanel.res kingpanel.rc
if errorlevel 1 goto buildfailed
"%ZIG_EXE%" cc -target x86_64-windows-gnu -Os -s -Wall -Wextra -Werror -Wl,--subsystem,windows kingpanel.c kingpanel.res -o KingPanel.exe -luser32 -lshell32 -lgdi32 -ladvapi32 -ldwmapi -lsetupapi
if errorlevel 1 goto buildfailed
del /q kingpanel.res >nul 2>nul
echo Built KingPanel.exe
exit /b 0
:buildfailed
exit /b 1
:installer
call "%~f0" app
if errorlevel 1 exit /b 1
if defined NSIS_EXE goto foundnsis
for /f "delims=" %%I in ('where makensis.exe 2^>nul') do if not defined NSIS_EXE set "NSIS_EXE=%%~fI"
if not defined NSIS_EXE if exist "%ProgramFiles(x86)%\NSIS\makensis.exe" set "NSIS_EXE=%ProgramFiles(x86)%\NSIS\makensis.exe"
if not defined NSIS_EXE if exist "%ProgramFiles%\NSIS\makensis.exe" set "NSIS_EXE=%ProgramFiles%\NSIS\makensis.exe"
if not defined NSIS_EXE (
 echo Install NSIS 3 or set NSIS_EXE to the full path to makensis.exe.
 exit /b 1
)
:foundnsis
"%NSIS_EXE%" /WX setup.nsi
exit /b
:usage
echo Usage: build.cmd [app^|installer]
exit /b 1
