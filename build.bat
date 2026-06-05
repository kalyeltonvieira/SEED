@echo off
setlocal
cd /d "%~dp0"

echo === SEED 1.0.0 - Compilando ===

REM Gerar build number (data atual)
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set datetime=%%I
set BUILD=%datetime:~0,8%
echo %BUILD% > D:\SEED\BUILD

echo Build: %BUILD%
echo Versão:
type D:\SEED\VERSION
echo.

echo [1/5] Compilando runtime C nativo...
cd runtime
call build.bat
cd ..
if not exist seed.exe (
  echo Erro ao compilar runtime C.
  exit /b 1
)

echo [2/5] Preparando base de conhecimento local...
if exist sqlite3.exe (
  sqlite3 knowledge.db < knowledge\schema.sql
) else (
  copy /Y knowledge\knowledge.seeddb knowledge.db >nul
)

echo [3/5] Registrando compilador inicial...
if not exist build mkdir build
copy /Y seed0.seed build\seed0.seed >nul

echo [4/5] Registrando geracao seed1...
copy /Y seed1.seed build\seed1.seed >nul

echo [5/5] Inicializando sonhos, testes e geracoes...
if not exist dreams mkdir dreams
if not exist generations mkdir generations
if not exist tests mkdir tests

echo.
echo [OK] seed.exe gerado
