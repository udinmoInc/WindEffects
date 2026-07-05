param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$CommandArgs
)

$ErrorActionPreference = "Stop"

$DescriptorFileName = "WindEffects.engine"

function Read-IniDocument {
    param([string]$Path)

    $global = @{}
    $sections = @{}
    $sectionName = $null

    foreach ($line in Get-Content -LiteralPath $Path) {
        $trimmed = $line.Trim()
        if ($trimmed.Length -eq 0 -or $trimmed.StartsWith("#")) { continue }

        if ($trimmed.StartsWith("[") -and $trimmed.EndsWith("]")) {
            $sectionName = $trimmed.Substring(1, $trimmed.Length - 2).Trim()
            if (-not $sections.ContainsKey($sectionName)) {
                $sections[$sectionName] = @{}
            }
            continue
        }

        $parts = $trimmed.Split("=", 2)
        if ($parts.Length -ne 2) { continue }

        $key = $parts[0].Trim()
        $value = $parts[1].Trim()

        if ([string]::IsNullOrWhiteSpace($sectionName)) {
            $global[$key] = $value
        } else {
            $sections[$sectionName][$key] = $value
        }
    }

    return @{ Global = $global; Sections = $sections }
}

function Find-EngineRoot {
    param([string[]]$StartDirectories)

    foreach ($start in $StartDirectories) {
        if ([string]::IsNullOrWhiteSpace($start)) { continue }
        $dir = $start
        if (Test-Path -LiteralPath $dir -PathType Container) {
            $dir = (Resolve-Path -LiteralPath $dir).Path
        }
        while ($true) {
            $descriptor = Join-Path $dir $DescriptorFileName
            if (Test-Path -LiteralPath $descriptor) {
                return @{ Root = $dir; Path = $descriptor }
            }
            $parent = Split-Path $dir -Parent
            if ([string]::IsNullOrWhiteSpace($parent) -or $parent -eq $dir) { break }
            $dir = $parent
        }
    }

    return $null
}

function Test-DotNetExecutable {
    param([string]$ExecutablePath)

    if (-not (Test-Path -LiteralPath $ExecutablePath)) { return $false }
    $dir = Split-Path -Parent $ExecutablePath
    $name = [System.IO.Path]::GetFileNameWithoutExtension($ExecutablePath)
    return (Test-Path -LiteralPath (Join-Path $dir "$name.runtimeconfig.json")) -and
           (Test-Path -LiteralPath (Join-Path $dir "$name.deps.json"))
}

function Invoke-Tool {
    param(
        [hashtable]$Tool,
        [string]$WorkingDirectory,
        [string[]]$ForwardArgs
    )

    $kind = $Tool["kind"]
    $executable = $Tool["executable"]
    $source = $Tool["source"]

    if ($kind -eq "dotnet") {
        if (-not [string]::IsNullOrWhiteSpace($executable) -and (Test-DotNetExecutable $executable)) {
            & $executable @ForwardArgs | Out-Host
            return $LASTEXITCODE
        }
        if (-not (Test-Path -LiteralPath $source)) {
            throw "Tool source project was not found: $source"
        }
        Push-Location $WorkingDirectory
        try {
            dotnet run --project $source -- @ForwardArgs | Out-Host
            return $LASTEXITCODE
        } finally {
            Pop-Location
        }
    }

    if (-not (Test-Path -LiteralPath $executable)) {
        throw "Tool executable was not found: $executable"
    }

    & $executable @ForwardArgs | Out-Host
    return $LASTEXITCODE
}

$startDirs = @(
    $env:WE_PROJECT_ROOT,
    $env:WE_ENGINE_ROOT,
    (Get-Location).Path,
    (Split-Path -Parent $MyInvocation.MyCommand.Path)
)

$installation = Find-EngineRoot $startDirs
if ($null -eq $installation) {
    Write-Error "Could not find $DescriptorFileName by searching upward from the current directory."
}

$engineRoot = $installation.Root
$descriptor = Read-IniDocument -Path $installation.Path

$programsRoot = Join-Path $engineRoot ($descriptor.Global["ProgramsRoot"])
$manifestRel = $descriptor.Global["bootstrap.manifest"]
$bootstrapEntrySource = $descriptor.Global["bootstrap.entry.source"]

if ([string]::IsNullOrWhiteSpace($manifestRel)) {
    Write-Error "Engine descriptor is missing bootstrap.manifest."
}

$manifestPath = Join-Path $engineRoot $manifestRel
$forwardArgs = $CommandArgs

if ($forwardArgs.Length -gt 0 -and $forwardArgs[0] -eq "build") {
    $probe = Join-Path $engineRoot "Build\Intermediate\IgniteBT\Launcher\we_probe.exe"
    if (Test-Path -LiteralPath $probe) {
        $buildArgs = @()
        if ($forwardArgs.Length -gt 1) {
            $buildArgs = $forwardArgs[1..($forwardArgs.Length - 1)]
        }
        & $probe @buildArgs
        if ($LASTEXITCODE -eq 0) { exit 0 }
        if ($LASTEXITCODE -ne 2) { exit $LASTEXITCODE }
    }
}

$tool = $null

if (Test-Path -LiteralPath $manifestPath) {
    $manifest = Read-IniDocument -Path $manifestPath
    $defaultTool = $manifest.Global["default"]

    if ($CommandArgs.Length -gt 0 -and $manifest.Sections.ContainsKey($CommandArgs[0])) {
        $tool = $manifest.Sections[$CommandArgs[0]]
    } elseif (-not [string]::IsNullOrWhiteSpace($defaultTool) -and $manifest.Sections.ContainsKey($defaultTool)) {
        $tool = $manifest.Sections[$defaultTool]
    }
}

if ($null -ne $tool) {
    $exitCode = Invoke-Tool -Tool $tool -WorkingDirectory $engineRoot -ForwardArgs $forwardArgs
    exit $exitCode
}

if ([string]::IsNullOrWhiteSpace($bootstrapEntrySource)) {
    Write-Error "Bootstrap manifest is unavailable and bootstrap.entry.source is missing."
}

$entryProject = Join-Path $programsRoot $bootstrapEntrySource
if (-not (Test-Path -LiteralPath $entryProject)) {
    Write-Error "Bootstrap entry project was not found: $entryProject"
}

Push-Location $engineRoot
try {
    dotnet run --project $entryProject -- @forwardArgs | Out-Host
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
