# Generate Windows VERSIONINFO resources from ProductMetadata.json
# This script reads the centralized product metadata and generates .rc files
# with VERSIONINFO blocks for each WindEffects executable.

param(
    [string]$ConfigPath = "Engine/Config/ProductMetadata.json"
)

$jsonBytes = [System.IO.File]::ReadAllBytes($ConfigPath)
# Detect and remove BOM if present
if ($jsonBytes.Length -ge 3 -and $jsonBytes[0] -eq 0xEF -and $jsonBytes[1] -eq 0xBB -and $jsonBytes[2] -eq 0xBF) {
    $jsonBytes = $jsonBytes[3..$jsonBytes.Length]
}
$jsonContent = [System.Text.Encoding]::UTF8.GetString($jsonBytes)
$json = $jsonContent | ConvertFrom-Json
# Force copyright to correct value
$json.company.copyright = "© 2026 Udinmo, Inc. All rights reserved."

$productName = $json.product.name
$productDisplayName = $json.product.displayName
$productVersion = $json.product.version
$productBuild = $json.product.build
$companyName = $json.company.name
$companyCopyright = $json.company.copyright
$engineGuid = $json.engine.guid
$engineBuildId = $json.engine.buildId

# Parse version components
$versionParts = $productVersion -split '\.'
$major = if ($versionParts.Length -gt 0) { [int]$versionParts[0] } else { 1 }
$minor = if ($versionParts.Length -gt 1) { [int]$versionParts[1] } else { 0 }
$patch = if ($versionParts.Length -gt 2) { [int]$versionParts[2] } else { 0 }
$build = if ($productBuild -match '^\d+$') { [int]$productBuild } else { 0 }

function Generate-VersionInfo {
    param(
        [string]$FileDescription,
        [string]$InternalName,
        [string]$OriginalFilename
    )

    @"
// Auto-generated VERSIONINFO resource from ProductMetadata.json
// DO NOT EDIT - regenerate with Scripts/Generate-VersionInfo.ps1

1 VERSIONINFO
 FILEVERSION $major,$minor,$patch,$build
 PRODUCTVERSION $major,$minor,$patch,$build
 FILEFLAGSMASK 0x3fL
#ifdef _DEBUG
 FILEFLAGS 0x1L
#else
 FILEFLAGS 0x0L
#endif
 FILEOS 0x40004L
 FILETYPE 0x1L
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904b0"
        BEGIN
            VALUE "CompanyName", "$companyName"
            VALUE "FileDescription", "$FileDescription"
            VALUE "FileVersion", "$productVersion.$productBuild"
            VALUE "InternalName", "$InternalName"
            VALUE "LegalCopyright", "COPYRIGHT_PLACEHOLDER"
            VALUE "OriginalFilename", "$OriginalFilename"
            VALUE "ProductName", "$productDisplayName"
            VALUE "ProductVersion", "$productVersion.$productBuild"
            VALUE "EngineId", "$engineGuid"
            VALUE "BuildId", "$engineBuildId"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x409, 1200
    END
END
"@
}

# Generate VERSIONINFO for each executable
$executables = @(
    @{ FileDescription = "WindEffects Editor"; InternalName = "WindeffectsEditor"; OriginalFilename = "WindeffectsEditor.exe"; Output = "Engine/Source/Programs/Editor/Resources/VERSIONINFO.rc" },
    @{ FileDescription = "WindEffects Launcher"; InternalName = "WeLauncher"; OriginalFilename = "WeLauncher.exe"; Output = "Engine/Source/Programs/WeLauncher/Resources/VERSIONINFO.rc" },
    @{ FileDescription = "WindEffects Crash Reporter"; InternalName = "WECrashReporter"; OriginalFilename = "WECrashReporter.exe"; Output = "Engine/Source/Programs/CrashReporter/Resources/VERSIONINFO.rc" },
    @{ FileDescription = "WindEffects Command-Line Tool"; InternalName = "we"; OriginalFilename = "we.exe"; Output = "Engine/Source/Programs/We/Resources/VERSIONINFO.rc" },
    @{ FileDescription = "WindEffects Asset Compiler"; InternalName = "WeAssetCompiler"; OriginalFilename = "WeAssetCompiler.exe"; Output = "Engine/Source/Programs/AssetCompiler/Resources/VERSIONINFO.rc" },
    @{ FileDescription = "WindEffects Shader Compiler"; InternalName = "WeShaderCompiler"; OriginalFilename = "WeShaderCompiler.exe"; Output = "Engine/Source/Programs/ShaderCompiler/Resources/VERSIONINFO.rc" },
    @{ FileDescription = "WindEffects Asset Cooker"; InternalName = "WeCooker"; OriginalFilename = "WeCooker.exe"; Output = "Engine/Source/Programs/Cooker/Resources/VERSIONINFO.rc" },
    @{ FileDescription = "WindEffects Packager"; InternalName = "WePackager"; OriginalFilename = "WePackager.exe"; Output = "Engine/Source/Programs/Packager/Resources/VERSIONINFO.rc" },
    @{ FileDescription = "WindEffects Reflection Hardening Tool"; InternalName = "ReflectionHardening"; OriginalFilename = "ReflectionHardening.exe"; Output = "Engine/Source/Programs/ReflectionHardening/Resources/VERSIONINFO.rc" },
    @{ FileDescription = "WindEffects Serialization Hardening Tool"; InternalName = "SerializationHardening"; OriginalFilename = "SerializationHardening.exe"; Output = "Engine/Source/Programs/SerializationHardening/Resources/VERSIONINFO.rc" }
)

foreach ($exe in $executables) {
    $versionInfo = Generate-VersionInfo -FileDescription $exe.FileDescription -InternalName $exe.InternalName -OriginalFilename $exe.OriginalFilename
    $outputPath = $exe.Output
    
    # Ensure output directory exists
    $outputDir = Split-Path $outputPath -Parent
    if (-not (Test-Path $outputDir)) {
        New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
    }
    
    # Write with UTF-8 no BOM encoding
    $utf8NoBom = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllText($outputPath, $versionInfo, $utf8NoBom)
    
    # Post-process to replace copyright placeholder with correct UTF-8 bytes
    $bytes = [System.IO.File]::ReadAllBytes($outputPath)
    $placeholder = [System.Text.Encoding]::ASCII.GetBytes("COPYRIGHT_PLACEHOLDER")
    $copyrightCorrect = [byte[]]@(0xC2, 0xA9, 0x20, 0x32, 0x30, 0x32, 0x36, 0x20, 0x55, 0x64, 0x69, 0x6e, 0x6d, 0x6f, 0x2c, 0x20, 0x49, 0x6e, 0x63, 0x2e, 0x20, 0x41, 0x6c, 0x6c, 0x20, 0x72, 0x69, 0x67, 0x68, 0x74, 0x73, 0x20, 0x72, 0x65, 0x73, 0x65, 0x72, 0x76, 0x65, 0x64, 0x2e)
    
    for ($i = 0; $i -le $bytes.Length - $placeholder.Length; $i++) {
        $match = $true
        for ($j = 0; $j -lt $placeholder.Length; $j++) {
            if ($bytes[$i + $j] -ne $placeholder[$j]) {
                $match = $false
                break
            }
        }
        if ($match) {
            $newBytes = [byte[]]::new($bytes.Length - $placeholder.Length + $copyrightCorrect.Length)
            [Array]::Copy($bytes, 0, $newBytes, 0, $i)
            [Array]::Copy($copyrightCorrect, 0, $newBytes, $i, $copyrightCorrect.Length)
            [Array]::Copy($bytes, $i + $placeholder.Length, $newBytes, $i + $copyrightCorrect.Length, $bytes.Length - $i - $placeholder.Length)
            [System.IO.File]::WriteAllBytes($outputPath, $newBytes)
            break
        }
    }
    
    Write-Host "Generated: $outputPath"
}



Write-Host "VERSIONINFO resources generated successfully."
