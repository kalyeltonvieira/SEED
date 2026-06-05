# SEED Installation Script
# This script sets up the SEED development environment and adds it to system PATH

Write-Host "=== SEED 1.0.0 - Installation ===" -ForegroundColor Green
Write-Host ""

# Get SEED installation directory
$seedPath = $PSScriptRoot
Write-Host "SEED Installation Directory: $seedPath" -ForegroundColor Yellow
Write-Host ""

# Check for GCC
Write-Host "Checking for GCC..." -ForegroundColor Yellow
$gccExists = Get-Command gcc -ErrorAction SilentlyContinue
if (-not $gccExists) {
    Write-Host "GCC not found in PATH." -ForegroundColor Red
    Write-Host "Please install MinGW-w64 or use Visual Studio Developer Command Prompt." -ForegroundColor Yellow
    Write-Host "Download from: https://www.mingw-w64.org/" -ForegroundColor Cyan
    exit 1
}
Write-Host "[OK] GCC found" -ForegroundColor Green

# Build runtime
Write-Host ""
Write-Host "Building runtime C..." -ForegroundColor Yellow
Set-Location runtime
& .\build.bat
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Failed to build runtime" -ForegroundColor Red
    Set-Location ..
    exit 1
}
Set-Location ..
Write-Host "[OK] Runtime built successfully" -ForegroundColor Green

# Verify seed.exe
Write-Host ""
Write-Host "Verifying seed.exe..." -ForegroundColor Yellow
if (-not (Test-Path "seed.exe")) {
    Write-Host "[ERROR] seed.exe not found" -ForegroundColor Red
    exit 1
}
Write-Host "[OK] seed.exe exists" -ForegroundColor Green

# Test seed.exe
Write-Host ""
Write-Host "Testing seed.exe..." -ForegroundColor Yellow
& .\seed.exe version
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] seed.exe test failed" -ForegroundColor Red
    exit 1
}
Write-Host "[OK] seed.exe works" -ForegroundColor Green

# Add to system PATH
Write-Host ""
Write-Host "Adding SEED to system PATH..." -ForegroundColor Yellow
$currentPath = [Environment]::GetEnvironmentVariable("Path", "Machine")
if ($currentPath -notlike "*$seedPath*") {
    [Environment]::SetEnvironmentVariable("Path", "$currentPath;$seedPath", "Machine")
    Write-Host "[OK] Added to system PATH" -ForegroundColor Green
} else {
    Write-Host "[SKIP] Already in system PATH" -ForegroundColor Gray
}

# Set SEED_HOME environment variable
Write-Host ""
Write-Host "Setting SEED_HOME environment variable..." -ForegroundColor Yellow
[Environment]::SetEnvironmentVariable("SEED_HOME", $seedPath, "Machine")
Write-Host "[OK] SEED_HOME set to: $seedPath" -ForegroundColor Green

# Create directories
Write-Host ""
Write-Host "Creating directories..." -ForegroundColor Yellow
$dirs = @("build", "dreams", "generations", "tests")
foreach ($dir in $dirs) {
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir | Out-Null
        Write-Host "  Created: $dir" -ForegroundColor Gray
    }
}
Write-Host "[OK] Directories ready" -ForegroundColor Green

# Summary
Write-Host ""
Write-Host "=== Installation Complete ===" -ForegroundColor Green
Write-Host ""
Write-Host "SEED 1.0.0 is now installed!" -ForegroundColor Green
Write-Host ""
Write-Host "Runtime: C (222KB)" -ForegroundColor Cyan
Write-Host "Compiler: SEED (self-hosted)" -ForegroundColor Cyan
Write-Host "REPL: SEED (tools/seedrepl.seed)" -ForegroundColor Cyan
Write-Host ""
Write-Host "Installation Directory: $seedPath" -ForegroundColor Yellow
Write-Host "System PATH: Updated" -ForegroundColor Yellow
Write-Host "SEED_HOME: $seedPath" -ForegroundColor Yellow
Write-Host ""
Write-Host "You can now use 'seed' from any directory!" -ForegroundColor Green
Write-Host ""
Write-Host "Usage:" -ForegroundColor Yellow
Write-Host "  seed                       Mostra tela de boas-vindas" -ForegroundColor White
Write-Host "  seed run <arquivo>         Compila e executa arquivo .seed" -ForegroundColor White
Write-Host "  seed build <projeto>       Compila projeto" -ForegroundColor White
Write-Host "  seed test                  Roda testes" -ForegroundColor White
Write-Host "  seed version               Mostra versao" -ForegroundColor White
Write-Host "  arquivo.seed               Executa arquivo .seed direto" -ForegroundColor White
Write-Host ""
Write-Host "Note: You may need to restart your terminal for PATH changes to take effect." -ForegroundColor Yellow
Write-Host ""
Write-Host "Documentation: https://seed-lang.org" -ForegroundColor Cyan
