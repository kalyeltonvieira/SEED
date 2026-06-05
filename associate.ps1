# SEED File Association Setup
# Associates .seed files with seed.exe so they run on double-click

$seedPath = $PSScriptRoot
Write-Host "Configuring SEED File Association in HKCU (no Admin rights needed)..." -ForegroundColor Yellow
Write-Host "SEED Path: $seedPath" -ForegroundColor Gray

$regPathExt = "HKCU:\Software\Classes\.seed"
$regPathClass = "HKCU:\Software\Classes\SEEDFile"

try {
    # 1. Register the extension
    if (-not (Test-Path $regPathExt)) {
        New-Item $regPathExt -Force | Out-Null
    }
    Set-ItemProperty -Path $regPathExt -Name "(Default)" -Value "SEEDFile" -Force
    
    # 2. Register the class description
    if (-not (Test-Path $regPathClass)) {
        New-Item $regPathClass -Force | Out-Null
    }
    Set-ItemProperty -Path $regPathClass -Name "(Default)" -Value "SEED Source File" -Force
    
    # 3. Register default icon (seed.ico)
    $iconPath = Join-Path $regPathClass "DefaultIcon"
    if (-not (Test-Path $iconPath)) {
        New-Item $iconPath -Force | Out-Null
    }
    Set-ItemProperty -Path $iconPath -Name "(Default)" -Value "$seedPath\runtime\seed.ico" -Force
    
    # 4. Register open command: compile, run if success, and pause
    $cmdPath = Join-Path $regPathClass "shell\open\command"
    if (-not (Test-Path $cmdPath)) {
        New-Item $cmdPath -Force | Out-Null
    }
    $cmdString = "cmd.exe /c `"$seedPath\seed.exe`" `"%1`" && `"%1.exe`" & pause"
    Set-ItemProperty -Path $cmdPath -Name "(Default)" -Value $cmdString -Force
    
    Write-Host "[OK] .seed file association completed successfully!" -ForegroundColor Green
    Write-Host "Now you can double-click any .seed file to compile and execute it!" -ForegroundColor Green
} catch {
    Write-Error "Failed to configure registry association: $_"
}
