# Build Number Generator for WindEffects Engine
# Generates sequential build numbers based on git commit count or local counter

param(
    [string]$ConfigPath = "Engine/Config/ProductMetadata.json",
    [switch]$UseGit = $false,
    [switch]$Increment = $false
)

$ErrorActionPreference = "Stop"

function Get-GitCommitCount {
    try {
        $result = git rev-list --count HEAD 2>$null
        if ($LASTEXITCODE -eq 0) {
            return [int]$result
        }
    } catch {
        # Git not available or not a git repo
    }
    return $null
}

function Get-LocalBuildCounter {
    $counterFile = "Build/Database/build-counter.txt"
    $counterDir = Split-Path $counterFile -Parent
    
    if (-not (Test-Path $counterDir)) {
        New-Item -ItemType Directory -Path $counterDir -Force | Out-Null
    }
    
    if (Test-Path $counterFile) {
        $counter = Get-Content $counterFile -Raw
        return [int]$counter.Trim()
    }
    
    # Initialize with a reasonable starting number
    return 1000
}

function Set-LocalBuildCounter {
    param([int]$NewCounter)
    
    $counterFile = "Build/Database/build-counter.txt"
    $counterDir = Split-Path $counterFile -Parent
    
    if (-not (Test-Path $counterDir)) {
        New-Item -ItemType Directory -Path $counterDir -Force | Out-Null
    }
    
    $NewCounter | Out-File -FilePath $counterFile -Encoding utf8 -NoNewline
}

function Update-ProductMetadata {
    param(
        [string]$Path,
        [int]$BuildNumber
    )
    
    if (-not (Test-Path $Path)) {
        Write-Error "Product metadata file not found: $Path"
        return $false
    }
    
    try {
        $json = Get-Content $Path -Raw | ConvertFrom-Json
        
        # Update build number
        if ($json.PSObject.Properties.Name -contains "build") {
            $json.build = $BuildNumber.ToString()
        } elseif ($json.PSObject.Properties.Name -contains "product") {
            $json.product.build = $BuildNumber.ToString()
        }
        
        # Generate unique BuildId (timestamp-based)
        $timestamp = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
        $randomPart = Get-Random -Minimum 1000 -Maximum 9999
        if ($json.PSObject.Properties.Name -contains "engine") {
            $json.engine.buildId = "WE-$timestamp-$randomPart"
        }
        
        # Update build timestamp
        if ($json.PSObject.Properties.Name -contains "build") {
            $json.build.timestamp = [DateTimeOffset]::UtcNow.ToString("o")
        }
        
        # Get git commit and branch if available
        try {
            $gitCommit = git rev-parse --short HEAD 2>$null
            if ($LASTEXITCODE -eq 0) {
                if ($json.PSObject.Properties.Name -contains "build") {
                    $json.build.gitCommit = $gitCommit.Trim()
                }
            }
            
            $gitBranch = git rev-parse --abbrev-ref HEAD 2>$null
            if ($LASTEXITCODE -eq 0) {
                if ($json.PSObject.Properties.Name -contains "build") {
                    $json.build.gitBranch = $gitBranch.Trim()
                }
            }
            
            # Get full commit hash for build revision
            $gitFullCommit = git rev-parse HEAD 2>$null
            if ($LASTEXITCODE -eq 0) {
                if ($json.PSObject.Properties.Name -contains "build") {
                    $json.build.buildRevision = $gitFullCommit.Trim()
                }
            }
        } catch {
            # Git not available - leave empty
        }
        
        $json | ConvertTo-Json -Depth 10 | Out-File -FilePath $Path -Encoding utf8
        Write-Host "Updated build number to $BuildNumber in $Path"
        if ($json.PSObject.Properties.Name -contains "engine") {
            Write-Host "Generated BuildId: $($json.engine.buildId)"
        }
        return $true
    } catch {
        Write-Error "Failed to update product metadata: $_"
        return $false
    }
}

# Main logic
$buildNumber = $null

if ($UseGit) {
    $gitCount = Get-GitCommitCount
    if ($gitCount) {
        $buildNumber = $gitCount + 1000 # Offset to avoid conflicts
        Write-Host "Using git commit count: $gitCount (build number: $buildNumber)"
    } else {
        Write-Warning "Git not available, falling back to local counter"
    }
}

if (-not $buildNumber) {
    $buildNumber = Get-LocalBuildCounter
    Write-Host "Using local build counter: $buildNumber"
}

if ($Increment) {
    $buildNumber++
    Set-LocalBuildCounter -NewCounter $buildNumber
    Write-Host "Incremented build counter to: $buildNumber"
}

# Update the product metadata file
$success = Update-ProductMetadata -Path $ConfigPath -BuildNumber $buildNumber

# Regenerate VERSIONINFO resources after updating metadata
if ($success) {
    $versionInfoScript = Join-Path $PSScriptRoot "Generate-VersionInfo.ps1"
    if (Test-Path $versionInfoScript) {
        & $versionInfoScript
    }
    exit 0
} else {
    exit 1
}
