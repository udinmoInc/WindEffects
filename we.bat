@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0we.ps1" -- %*
exit /b %ERRORLEVEL%
