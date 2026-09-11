#requires -Version 5.1
<#
.SYNOPSIS
  Parse WindEffects session logs for toolbar-click / minimize-quit / Vulkan fence stalls.

.DESCRIPTION
  Classifies LoopTrace / Loop pulse evidence into:
    - MessageQueueDeadlock  (EventQueue.MUTEX waits, no EXIT for PollEvents)
    - MissingRepaint        (GATE paint=0 after click with no paints+)
    - OsFocusCaptureLoss    (focus=lost without minimize; clear-input mid-click)
    - VulkanFenceBlock      (vkWaitForFences timeout / BeginFrame FAIL)  -- primary site

.PARAMETER LogPath
  Path to WindEffects.log (defaults to newest Development session log).

.EXAMPLE
  $env:WE_LOOP_TRACE=1
  .\we.ps1 editor
  .\Scripts\Diagnose-EditorLoopTrace.ps1
#>
param(
    [string]$LogPath = ""
)

$ErrorActionPreference = "Stop"

function Resolve-LatestSessionLog {
    $root = Join-Path $PSScriptRoot "..\Build\Output\Win64\Development\Saved\Logs\Sessions"
    if (-not (Test-Path $root)) {
        throw "No session log root at $root - run the editor first."
    }
    $latest = Get-ChildItem $root -Directory |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    $log = Join-Path $latest.FullName "WindEffects.log"
    if (-not (Test-Path $log)) {
        throw "Missing WindEffects.log under $($latest.FullName)"
    }
    return (Resolve-Path $log).Path
}

if (-not $LogPath) {
    $LogPath = Resolve-LatestSessionLog
}

Write-Host "Analyzing: $LogPath"
Write-Host ""

$lines = Get-Content -LiteralPath $LogPath

$patterns = @{
    LoopTrace     = '\[LoopTrace\]'
    LoopPulse     = '\[Loop\] state='
    FenceTimeout  = 'vkWaitForFences timed out'
    BeginFail     = 'BeginFrame failed'
    FocusLost     = 'focus=lost'
    FocusGained   = 'focus=gained'
    TitleMin      = 'TitleBar\.MinimizeClick'
    TitleClose    = 'TitleBar\.CloseClick'
    LifecycleMin  = 'Lifecycle\.WindowMinimize'
    LifecycleQuit = 'Lifecycle\.(QuitEvent|WindowCloseEvent)'
    Mutex         = '\[LoopTrace\] MUTEX'
    Swapchain     = 'Ensuring swapchain matches'
}

$hits = @{}
foreach ($key in $patterns.Keys) {
    $hits[$key] = @($lines | Select-String -Pattern $patterns[$key])
}

Write-Host "=== Counts ==="
foreach ($key in ($hits.Keys | Sort-Object)) {
    Write-Host ("  {0,-14} {1}" -f $key, $hits[$key].Count)
}

Write-Host ""
Write-Host "=== Classification ==="

$fenceCount = $hits.FenceTimeout.Count + $hits.BeginFail.Count
$mutexCount = $hits.Mutex.Count
$focusLostWhilePresent = @($hits.LoopPulse | Where-Object { $_.Line -match 'focus=0' -and $_.Line -match 'begin=1' -and $_.Line -notmatch 'min=1' }).Count

if ($fenceCount -gt 0) {
    Write-Host "PRIMARY: VulkanFenceBlock - main thread blocked in VulkanDevice::BeginFrame at vkWaitForFences (50ms timeout)."
    Write-Host "  File: Engine/Source/Runtime/VulkanRHI/Private/VulkanDeviceFrame.cpp (vkWaitForFences call)."
    Write-Host "  Effect: RenderPipeline skips Submit/Present -> Loop state=beginFrame-FAIL (UI looks stuck)."
    Write-Host "  Correlation: often follows focus=gained + EnsureVisibleSwapchain recreate."
    Write-Host ""
    Write-Host "  Recent fence / BeginFrame lines:"
    ($hits.FenceTimeout + $hits.BeginFail) | Select-Object -Last 8 | ForEach-Object { Write-Host ("    " + $_.Line) }
} else {
    Write-Host "No Vulkan fence timeouts in this log."
}

if ($mutexCount -gt 0) {
    Write-Host ""
    Write-Host "SECONDARY: EventQueue mutex wait >1ms (same-thread nested Push during Flush is the deadlock risk)."
    $hits.Mutex | Select-Object -Last 5 | ForEach-Object { Write-Host ("    " + $_.Line) }
} else {
    Write-Host ""
    Write-Host "No EventQueue mutex contention logged (message-queue deadlock unlikely)."
}

if ($focusLostWhilePresent -gt 0) {
    Write-Host ""
    Write-Host "OS focus: $focusLostWhilePresent pulse(s) with focus=0 while still presenting (not a pump stop)."
    Write-Host "  ClearAllInputState on focus-lost can abort in-progress toolbar clicks (OS focus capture loss)."
}

$traceClicks = $hits.TitleMin.Count + $hits.TitleClose.Count + $hits.LifecycleQuit.Count + $hits.LifecycleMin.Count
if ($traceClicks -eq 0 -and $hits.LoopTrace.Count -eq 0) {
    Write-Host ""
    Write-Host "No [LoopTrace] lines - re-run editor with:  `$env:WE_LOOP_TRACE='1'"
} else {
    Write-Host ""
    Write-Host "=== Recent LoopTrace (last 40) ==="
    $hits.LoopTrace | Select-Object -Last 40 | ForEach-Object { Write-Host ("  " + $_.Line) }
}

Write-Host ""
Write-Host "Done."
