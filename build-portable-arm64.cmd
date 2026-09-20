@echo off
setlocal EnableExtensions

rem Double-click to build the ARM64 portable using the existing build script.
rem Keep this file beside build-portable.cmd in the repository root.
rem Pass "nopause" when running in CI/automation.
rem Keep arm64 last so this entry point always selects ARM64.
call "%~dp0build-portable.cmd" %* arm64
exit /b %errorlevel%
