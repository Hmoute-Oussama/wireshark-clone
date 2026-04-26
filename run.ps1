$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$qtBin = "C:\Qt\6.10.2\mingw_64\bin"
$mingwBin = "C:\msys64\mingw64\bin"
$exePath = Join-Path $repoRoot "cmake-build-debug\WiresharkClone.exe"

if (-not (Test-Path $exePath)) {
    Write-Error "Executable not found: $exePath"
}

$env:PATH = "$qtBin;$mingwBin;$env:PATH"
Start-Process -FilePath $exePath -WorkingDirectory (Split-Path -Parent $exePath)
