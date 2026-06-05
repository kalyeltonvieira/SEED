@echo off
setlocal
cd /d "%~dp0"

echo === SEED 1.0.0 - Compilando Runtime C ===

REM Check for GCC
where gcc >nul 2>nul
if errorlevel 1 (
    echo GCC nao encontrado no PATH.
    echo Instale MinGW ou use MSVC.
    echo.
    echo Para instalar MinGW: https://www.mingw-w64.org/
    echo Ou use Visual Studio Developer Command Prompt.
    exit /b 1
)

REM Compile runtime with shlwapi for path functions
echo Compilando recursos...
call windres seed.rc -O coff -o seed.res
if errorlevel 1 (
    echo [ERRO] Falha ao compilar recursos [seed.rc]
    exit /b 1
)

echo Compilando seed_runtime.cpp...
call g++ -std=c++23 -O2 -o ..\seed.exe seed_runtime.cpp seed.res -luser32 -lkernel32 -lshlwapi
if errorlevel 1 (
    echo [ERRO] Falha ao compilar seed_runtime.cpp
    exit /b 1
)

echo.
echo [OK] seed.exe gerado (Runtime C)
echo.
echo Tamanho:
dir ..\seed.exe | find "seed.exe"
echo.
echo O runtime C esta pronto.
echo Para usar: .\seed.exe ^<arquivo.seedc^>
echo.
echo O seed.exe agora funciona de qualquer lugar do sistema.
