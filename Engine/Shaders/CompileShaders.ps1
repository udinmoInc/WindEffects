# Compiles HLSL viewport shaders to SPIR-V for Vulkan.
param(
    [string]$EngineRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$OutputDir = (Join-Path $PSScriptRoot "Bytecodes")
)

$ErrorActionPreference = "Stop"

function Find-Dxc {
    $cmd = Get-Command dxc -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $wingetRoot = Join-Path $env:LOCALAPPDATA "Microsoft\WinGet\Packages"
    if (Test-Path $wingetRoot) {
        $candidate = Get-ChildItem -Path $wingetRoot -Recurse -Filter "dxc.exe" -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match "\\x64\\dxc\.exe$" } |
            Select-Object -First 1
        if ($candidate) { return $candidate.FullName }
    }

    throw "DXC not found. Install the DirectX Shader Compiler (dxc.exe) and ensure it is on PATH."
}

$dxc = Find-Dxc
$includeRoot = Join-Path $EngineRoot "Shaders"
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$shaders = @(
    @{ Name = "UI"; Path = "Rendering/UI.hlsl" },
    @{ Name = "TextMSDF"; Path = "Rendering/TextMSDF.hlsl" },
    @{ Name = "TextSDF"; Path = "Rendering/TextSDF.hlsl" },
    @{ Name = "TextBitmap"; Path = "Rendering/TextBitmap.hlsl" },
    @{ Name = "TextMTSDF"; Path = "Rendering/TextMTSDF.hlsl" },
    @{ Name = "EditorGrid"; Path = "Editor/EditorGrid.hlsl" },
    @{ Name = "AtmospherePass"; Path = "Rendering/AtmospherePass.hlsl" },
    @{ Name = "VolumetricCloudsPass"; Path = "Rendering/VolumetricCloudsPass.hlsl" },
    @{ Name = "CloudTemporalResolve"; Path = "Rendering/CloudTemporalResolve.hlsl" },
    @{ Name = "CloudCompositePass"; Path = "Rendering/CloudCompositePass.hlsl" },
    @{ Name = "FogCompositePass"; Path = "Rendering/FogCompositePass.hlsl" },
    @{ Name = "SceneObject"; Path = "Rendering/SceneObject.hlsl" },
    @{ Name = "ProceduralSky"; Path = "Foundation/ProceduralSky.hlsl" },
    @{ Name = "BasicMesh"; Path = "Foundation/BasicMesh.hlsl" },
    @{ Name = "PostExposureCS"; Path = "Compute/PostExposureCS.hlsl"; Compute = $true },
    @{ Name = "PostCompositeCS"; Path = "Compute/PostCompositeCS.hlsl"; Compute = $true },
    @{ Name = "LuminanceReduceCS"; Path = "Compute/LuminanceReduceCS.hlsl"; Compute = $true },
    @{ Name = "LuminanceAvgCS"; Path = "Compute/LuminanceAvgCS.hlsl"; Compute = $true },
    @{ Name = "BloomPrefilterCS"; Path = "Compute/BloomPrefilterCS.hlsl"; Compute = $true },
    @{ Name = "BloomBlurCS"; Path = "Compute/BloomBlurCS.hlsl"; Compute = $true }
)

foreach ($shader in $shaders) {
    $source = Join-Path $includeRoot $shader.Path
    if (-not (Test-Path $source)) {
        throw "Shader source missing: $source"
    }

    if ($shader.Compute) {
        $csOut = Join-Path $OutputDir "$($shader.Name)_CS.spv"
        & $dxc -spirv -T cs_6_0 -E CSMain -I $includeRoot -Fo $csOut $source
        if ($LASTEXITCODE -ne 0) { throw "Failed to compile CS for $($shader.Name)" }
        Write-Host "Compiled $($shader.Name) (compute)"
        continue
    }

    $vsOut = Join-Path $OutputDir "$($shader.Name)_VS.spv"
    $psOut = Join-Path $OutputDir "$($shader.Name)_PS.spv"

    & $dxc -spirv -T vs_6_0 -E VSMain -I $includeRoot -Fo $vsOut $source
    if ($LASTEXITCODE -ne 0) { throw "Failed to compile VS for $($shader.Name)" }

    & $dxc -spirv -T ps_6_0 -E PSMain -I $includeRoot -Fo $psOut $source
    if ($LASTEXITCODE -ne 0) { throw "Failed to compile PS for $($shader.Name)" }

    Write-Host "Compiled $($shader.Name)"
}

$copyTargets = @(
    (Join-Path $EngineRoot "..\Build\Output\Win64\Debug\Engine\Shaders\Bytecodes"),
    (Join-Path $EngineRoot "..\Build\Output\Win64\Development\Engine\Shaders\Bytecodes"),
    (Join-Path $EngineRoot "..\Build\Output\Win64\Shipping\Engine\Shaders\Bytecodes"),
    (Join-Path $EngineRoot "..\Assets\Shaders")
)

foreach ($target in $copyTargets) {
    if (-not (Test-Path (Split-Path $target -Parent))) { continue }
    New-Item -ItemType Directory -Force -Path $target | Out-Null
    Get-ChildItem -Path (Join-Path $OutputDir "*.spv") | ForEach-Object {
        try {
            Copy-Item -Path $_.FullName -Destination $target -Force -ErrorAction Stop
        } catch {
            Write-Warning "Could not copy $($_.Name) to $target (file may be in use - restart the editor and re-run CompileShaders.ps1)."
        }
    }
    Write-Host "Synced bytecodes to $target"
}

Write-Host "Shader bytecodes written to $OutputDir"
