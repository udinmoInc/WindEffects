$ErrorActionPreference = "Stop"
$src = Join-Path $PSScriptRoot "we_probe.c"
$outDir = Join-Path $PSScriptRoot "..\..\..\..\Build\Intermediate\IgniteBT\Launcher"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$out = Join-Path $outDir "we_probe.exe"

$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if (-not $cl) {
    $vsCl = @(
        "F:\vs\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\*\bin\Hostx64\x64\cl.exe"
    ) | ForEach-Object { Get-Item $_ -ErrorAction SilentlyContinue } | Select-Object -First 1
    if ($vsCl) { $cl = $vsCl }
}
if ($cl) {
    Push-Location $PSScriptRoot
    try {
        $clPath = if ($cl -is [System.IO.FileInfo]) { $cl.FullName } elseif ($cl -is [System.Management.Automation.ApplicationInfo]) { $cl.Source } else { [string]$cl }
        $vcvarsCandidates = @(
            "F:\vs\VC\Auxiliary\Build\vcvars64.bat",
            "$env:ProgramFiles\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
            "$env:ProgramFiles\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
        )
        $vcvars = $vcvarsCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
        if ($vcvars) {
            $outEsc = $out.Replace('"', '""')
            cmd /c "`"$vcvars`" >nul && `"$clPath`" /nologo /O2 /Fe:`"$outEsc`" we_probe.c"
        } else {
            & "$clPath" /nologo /O2 "/Fe:$out" we_probe.c
        }
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
