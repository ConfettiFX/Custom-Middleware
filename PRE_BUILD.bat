@echo off
REM Change active working directory in case we run script for outside of TheForge
cd /D "%~dp0"

echo Syncing The-Forge

git clone https://github.com/ConfettiFX/The-Forge.git

exit /b 0