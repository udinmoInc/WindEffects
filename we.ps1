param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Args
)

$ErrorActionPreference = "Stop"

function Test-IgniteBtExecutable {
    param([string]$ExecutablePath)

    if (-not (Test-Path $ExecutablePath)) {
        return $false
    }

    $outputDir = Split-Path -Parent $ExecutablePath
    $binaryName = [System.IO.Path]::GetFileNameWithoutExtension($ExecutablePath)
    $runtimeConfig = Join-Path $outputDir "$binaryName.runtimeconfig.json"
    $depsJson = Join-Path $outputDir "$binaryName.deps.json"
    return (Test-Path $runtimeConfig) -and (Test-Path $depsJson)
}

$LauncherDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = if ($env:WE_PROJECT_ROOT) { $env:WE_PROJECT_ROOT } else { $LauncherDir }
$EngineRoot = if ($env:WE_ENGINE_ROOT) { $env:WE_ENGINE_ROOT } else { Join-Path $ProjectRoot "Engine" }

$IgniteBtProject = Join-Path $EngineRoot "Source\Programs\IgniteBT\IgniteBT.csproj"
$Configurations = @("Debug", "Release", "Development", "Shipping")

foreach ($config in $Configurations) {
    $candidate = Join-Path $ProjectRoot "Build\Intermediate\IgniteBT\$config\net8.0\IgniteBT.exe"
    if (Test-IgniteBtExecutable $candidate) {
        & $candidate @Args
        exit $LASTEXITCODE
    }
}

if (-not (Test-Path $IgniteBtProject)) {
    Write-Error "IgniteBT project not found at $IgniteBtProject. Run from the WindEffects repository root or set WE_PROJECT_ROOT."
}

Push-Location $ProjectRoot
try {
    dotnet run --project $IgniteBtProject -- @Args
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
