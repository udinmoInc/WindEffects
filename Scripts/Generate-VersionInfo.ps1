# Generate Windows VERSIONINFO resources from ProductMetadata.json
# This script reads the centralized product metadata and generates .rc files
# with VERSIONINFO blocks for each WindEffects executable.

param(
    [string]$ConfigPath = "Engine/Config/ProductMetadata.json"
)

$json = Get-Content $ConfigPath -Raw | ConvertFrom-Json

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
            VALUE "LegalCopyright", "$companyCopyright"
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
    
    $versionInfo | Out-File -FilePath $outputPath -Encoding UTF8
    Write-Host "Generated: $outputPath"
}

Write-Host "VERSIONINFO resources generated successfully."
