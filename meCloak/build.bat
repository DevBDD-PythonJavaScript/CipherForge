@echo off
title meCloak Build
color 0A

echo ==========================================
echo   meCloak - Build Script
echo ==========================================
echo.

where g++ >nul 2>&1
if errorlevel 1 (
    echo [ERROR] g++ not found! Install MinGW-w64 first.
    echo Download: https://winlibs.com
    pause
    exit /b 1
)

for /f "tokens=3" %%v in ('g++ --version ^| findstr "g++"') do set GCC_VER=%%v
echo [OK] Compiler: g++ %GCC_VER%
echo.

if exist "meCloak.exe" del "meCloak.exe"
if exist "*.o" del "*.o"

echo [*] Compiling (release, size optimized)...

g++ -static -O2 -s -mwindows main.cpp -o meCloak.exe -lcomctl32 ^
    -ffunction-sections ^
    -fdata-sections ^
    -Wl,--gc-sections ^
    -Wl,--strip-all ^
    -fno-ident ^
    -fno-asynchronous-unwind-tables ^
    -fomit-frame-pointer

if exist "meCloak.exe" (
    echo.
    echo ==========================================
    echo   [SUCCESS] meCloak.exe created!
    echo ==========================================
    for %%A in ("meCloak.exe") do echo   Size: %%~zA bytes
    echo.
    echo   Run: meCloak.exe
) else (
    echo.
    echo ==========================================
    echo   [FAILED] Compilation error!
    echo ==========================================
)

echo.
pause