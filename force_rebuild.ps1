# Force rebuild of modified modules by deleting their object files
$ErrorActionPreference = "Stop"

$modulesToRebuild = @(
    "VulkanRHI",
    "Editor", 
    "Core",
    "Platform"
)

Write-Host "Forcing rebuild of modified modules..." -ForegroundColor Yellow

foreach ($module in $modulesToRebuild) {
    $objPath = "Build\Intermediate\Win64\Development\Intermediate\$module"
    if (Test-Path $objPath) {
        Write-Host "Deleting object files for $module..." -ForegroundColor Cyan
        Remove-Item -Recurse -Force $objPath -ErrorAction SilentlyContinue
    }
    
    $importLibPath = "Build\Intermediate\Win64\Development\ImportLibs\$module"
    if (Test-Path $importLibPath) {
        Write-Host "Deleting import libs for $module..." -ForegroundColor Cyan
        Remove-Item -Recurse -Force $importLibPath -ErrorAction SilentlyContinue
    }
}

# Also delete the main editor executable and dlls
Write-Host "Deleting main executables and DLLs..." -ForegroundColor Cyan
Remove-Item -Force "Build\Output\Win64\Development\WindEffectsEditor.exe" -ErrorAction SilentlyContinue
Remove-Item -Force "Build\Output\Win64\Development\WindeffectsEditor.dll" -ErrorAction SilentlyContinue
Remove-Item -Force "Build\Output\Win64\Development\WindeffectsPlatform.dll" -ErrorAction SilentlyContinue
Remove-Item -Force "Build\Output\Win64\Development\WindeffectsCore.dll" -ErrorAction SilentlyContinue
Remove-Item -Force "Build\Output\Win64\Development\WERenderer.dll" -ErrorAction SilentlyContinue
Remove-Item -Force "Build\Output\Win64\Development\WERHI.dll" -ErrorAction SilentlyContinue
Remove-Item -Force "Build\Output\Win64\Development\WEVulkanRHI.dll" -ErrorAction SilentlyContinue

Write-Host "Force rebuild preparation complete. Now run: .\we.ps1 build --config Development" -ForegroundColor Green
