@echo off
setlocal
cd /d "%~dp0"

echo ==============================================
echo  Building autoLyrica (Release x64)
echo ==============================================

cmake -B build -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configure failed.
    pause
    exit /b %ERRORLEVEL%
)

cmake --build build --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo [SUCCESS] autoLyrica built successfully!
echo Executable: build\Release\autoLyrica.exe
pause
