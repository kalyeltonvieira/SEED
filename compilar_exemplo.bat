@echo off
REM Script para compilar arquivo SEED
REM Use: compilar_exemplo.bat nome_do_arquivo.seed

setlocal enabledelayedexpansion

if "%~1"=="" (
    echo Uso: compilar_exemplo.bat arquivo.seed
    echo.
    echo Exemplo: compilar_exemplo.bat exemplo_seed.seed
    pause
    exit /b 1
)

set ARQUIVO=%~1
set SAIDA=%~n1.bin

if not exist "%ARQUIVO%" (
    echo Erro: Arquivo "%ARQUIVO%" nao encontrado.
    pause
    exit /b 1
)

if not exist "seed.exe" (
    echo Erro: seed.exe nao encontrado no diretorio atual.
    echo Execute build.bat primeiro.
    pause
    exit /b 1
)

echo Compilando %ARQUIVO% para %SAIDA%...
seed.exe "%ARQUIVO%" -o "%SAIDA%"

if errorlevel 1 (
    echo.
    echo Compilacao falhou!
    pause
    exit /b 1
)

echo.
echo Compilacao concluida com sucesso!
echo Arquivo gerado: %SAIDA%
pause
