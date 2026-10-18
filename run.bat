@echo off
setlocal
cd /d "%~dp0"

echo Launching autoLyrica Desktop App...
start "" "build\Release\autoLyrica.exe"
