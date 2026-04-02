@echo off
setlocal

set SCRIPT_DIR=%~dp0
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%redeploy_fender_studio_pro.ps1" -PurgeHostCache %*

endlocal
