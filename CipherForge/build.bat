@echo off
title CipherForge Build
color 0A

echo ==========================================
echo   CipherForge - Build Script
echo ==========================================
echo.

:: Check if g++ is available
where g++ >nul 2>&1
if errorlevel 1 (
    echo [ERROR] g++ not found! Install MinGW-w64 first.
    echo Download: https://winlibs.com
    pause
    exit /b 1
)

:: Get g++ version
for /f "tokens=3" %%v in ('g++ --version ^| findstr "g++"') do set GCC_VER=%%v
echo [OK] Compiler: g++ %GCC_VER%
echo.

:: Clean old build
if exist "CipherForge.exe" del "CipherForge.exe"
if exist "*.o" del "*.o"

:: Compile
echo [*] Compiling...
g++ -static -O2 -mwindows main.cpp -o CipherForge.exe -lcomctl32

:: Check result
if exist "CipherForge.exe" (
    echo.
    echo ==========================================
    echo   [SUCCESS] CipherForge.exe created!
    echo ==========================================
    for %%A in ("CipherForge.exe") do echo   Size: %%~zA bytes
    echo.
    echo   Run: CipherForge.exe
) else (
    echo.
    echo ==========================================
    echo   [FAILED] Compilation error!
    echo ==========================================
)

echo.
pause