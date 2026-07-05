$ErrorActionPreference = "Stop"
$src = Join-Path $PSScriptRoot "we_probe.c"
$outDir = Join-Path $PSScriptRoot "..\..\..\..\Build\Intermediate\IgniteBT\Launcher"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$out = Join-Path $outDir "we_probe.exe"

$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Push-Location $PSScriptRoot
    try {
        & cl.exe /nologo /O2 /Fe:$out we_probe.c
        if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }
        Write-Host "Built $out"
        exit 0
    } finally {
        Pop-Location
        Remove-Item we_probe.obj -ErrorAction SilentlyContinue
    }
}

$gcc = Get-Command gcc -ErrorAction SilentlyContinue
if ($gcc) {
    & gcc -O2 -o (Join-Path $outDir "we_probe") $src
    Write-Host "Built $(Join-Path $outDir 'we_probe')"
    exit 0
}

Write-Warning "No C compiler found (cl.exe or gcc). Skipping we_probe build."
exit 0
