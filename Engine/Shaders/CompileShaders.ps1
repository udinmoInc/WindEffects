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
    @{ Name = "EditorGrid"; Path = "Editor/EditorGrid.hlsl" },
    @{ Name = "AtmospherePass"; Path = "Rendering/AtmospherePass.hlsl" },
    @{ Name = "VolumetricCloudsPass"; Path = "Rendering/VolumetricCloudsPass.hlsl" },
    @{ Name = "FogCompositePass"; Path = "Rendering/FogCompositePass.hlsl" },
    @{ Name = "SceneObject"; Path = "Rendering/SceneObject.hlsl" }
)

foreach ($shader in $shaders) {
    $source = Join-Path $includeRoot $shader.Path
    if (-not (Test-Path $source)) {
        throw "Shader source missing: $source"
    }

    $vsOut = Join-Path $OutputDir "$($shader.Name)_VS.spv"
    $psOut = Join-Path $OutputDir "$($shader.Name)_PS.spv"

    & $dxc -spirv -T vs_6_0 -E VSMain -I $includeRoot -Fo $vsOut $source
    if ($LASTEXITCODE -ne 0) { throw "Failed to compile VS for $($shader.Name)" }

    & $dxc -spirv -T ps_6_0 -E PSMain -I $includeRoot -Fo $psOut $source
    if ($LASTEXITCODE -ne 0) { throw "Failed to compile PS for $($shader.Name)" }

    Write-Host "Compiled $($shader.Name)"
}

Write-Host "Shader bytecodes written to $OutputDir"
