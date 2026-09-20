@echo off
setlocal EnableExtensions

rem Double-click to build the ARM64 installer using the existing build script.
rem Keep this file beside build-installer.cmd in the repository root.
rem Pass "nopause" when running in CI/automation.
rem Keep arm64 last so this entry point always selects ARM64.
call "%~dp0build-installer.cmd" %* arm64
exit /b %errorlevel%
