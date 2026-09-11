#requires -Version 5.1
<#
.SYNOPSIS
  Automated editor-loop diagnostic: launch, inject focus/minimize lifecycle, tail logs, report.

.DESCRIPTION
  1. Launches WindeffectsEditor.exe with WE_LOOP_TRACE=1 (verbose loop telemetry).
  2. Within the first ~10 presented frames, injects focus-lost/gained + minimize/restore
     via Win32, then posts WM_CLOSE (taskbar-close equivalent).
  3. Tails the session WindEffects.log in real time for:
       - assertion / fatal / exception lines
       - vkWaitForFences timeouts / BeginFrame failures
       - message-pump / Lifecycle.Quit / WindowClose stalls
  4. Writes a structured JSON telemetry report with file/line/subsystem hang evidence.

.PARAMETER TimeoutSec
  Max wall time for the whole run (default 90).

.PARAMETER KeepAliveSec
  Seconds to run after lifecycle injection before forced close (default 8).

.PARAMETER SkipLaunch
  Analyze an already-running editor / newest log only (no process start).

.EXAMPLE
  .\Scripts\Test-EditorLoopLifecycle.ps1
#>
param(
    [int]$TimeoutSec = 90,
    [int]$KeepAliveSec = 8,
    [switch]$SkipLaunch,
    [string]$ReportPath = ""
)

$ErrorActionPreference = "Stop"
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$EditorExe = Join-Path $ProjectRoot "Build\Output\Win64\Development\WindeffectsEditor.exe"
$SessionsRoot = Join-Path $ProjectRoot "Build\Output\Win64\Development\Saved\Logs\Sessions"
$ReportDir = Join-Path $ProjectRoot "Build\Output\Win64\Development\Saved\Logs\Diagnostics"
if (-not $ReportPath) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $ReportPath = Join-Path $ReportDir "EditorLoopLifecycle_$stamp.json"
}

Add-Type @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class WeWin32Diag {
    public const int SW_MINIMIZE = 6;
    public const int SW_RESTORE = 9;
    public const int SW_SHOW = 5;
    public const uint WM_CLOSE = 0x0010;
    public const uint WM_SYSCOMMAND = 0x0112;
    public const uint WM_ACTIVATE = 0x0006;
    public const uint WA_INACTIVE = 0;
    public const uint WA_ACTIVE = 1;
    public const int SC_MINIMIZE = 0xF020;
    public const int SC_RESTORE = 0xF120;
    public const int SC_CLOSE = 0xF060;
    public delegate bool EnumProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint windowPid);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr hWnd);
}
"@
# Alias type name used throughout the script


function Write-Step([string]$msg) {
    Write-Host ("[{0:HH:mm:ss.fff}] {1}" -f (Get-Date), $msg)
}

function Find-EditorHwnd([int]$processId) {
    $script:weEnumTargetPid = [uint32]$processId
    $callback = [WeWin32Diag+EnumProc]{
        param([IntPtr]$hWnd, [IntPtr]$lParam)
        [uint32]$windowPid = 0
        [void][WeWin32Diag]::GetWindowThreadProcessId($hWnd, [ref]$windowPid)
        if ($windowPid -eq $script:weEnumTargetPid -and [WeWin32Diag]::IsWindowVisible($hWnd)) {
            $sb = New-Object System.Text.StringBuilder 512
            [void][WeWin32Diag]::GetWindowText($hWnd, $sb, $sb.Capacity)
            $title = $sb.ToString()
            if ($title -match "WindEffects") {
                $script:foundHwnd = $hWnd
                return $false
            }
        }
        return $true
    }
    $script:foundHwnd = [IntPtr]::Zero
    [void][WeWin32Diag]::EnumWindows($callback, [IntPtr]::Zero)
    return $script:foundHwnd
}

function Wait-NewSessionLog([datetime]$notBefore, [int]$timeoutSec) {
    $deadline = (Get-Date).AddSeconds($timeoutSec)
    while ((Get-Date) -lt $deadline) {
        if (Test-Path $SessionsRoot) {
            $dirs = Get-ChildItem $SessionsRoot -Directory | Sort-Object LastWriteTime -Descending
            foreach ($d in $dirs) {
                if ($d.LastWriteTime -lt $notBefore.AddSeconds(-2)) { continue }
                $log = Join-Path $d.FullName "WindEffects.log"
                if (Test-Path $log) {
                    return (Resolve-Path $log).Path
                }
            }
        }
        Start-Sleep -Milliseconds 200
    }
    throw "Timed out waiting for a new session WindEffects.log under $SessionsRoot"
}

function Get-LogFrame([string]$line) {
    if ($line -match '\[Frame:(\d+)\]') { return [int]$Matches[1] }
    return -1
}

function New-TelemetryState {
    return [ordered]@{
        startedAtUtc        = (Get-Date).ToUniversalTime().ToString("o")
        editorExe           = $EditorExe
        logPath             = $null
        pid                 = $null
        hwnd                = $null
        lifecycleInjected   = $false
        injectionAtFrame    = $null
        exitCode            = $null
        hangSuspected       = $false
        hangIdleSec         = 0
        pendingProbe        = $null
        classifications     = @()
        findings            = @()
        counts              = [ordered]@{
            loopTrace           = 0
            fenceTimeout        = 0
            beginFrameFail      = 0
            assertion           = 0
            fatalException      = 0
            focusGained         = 0
            focusLost           = 0
            minimize            = 0
            windowClose         = 0
            quitEvent           = 0
            swapchainRequested  = 0
            swapchainEnsure     = 0
            mutexWait           = 0
            pulsePresented      = 0
            pulseBeginFail      = 0
        }
        lastLoopPulse       = $null
        lastInterestingLine = $null
        hangSite            = $null
        verdict             = "UNKNOWN"
    }
}

function Add-Finding($state, [string]$kind, [string]$severity, [string]$file = $null, [string]$line = $null, [string]$subsystem = $null, [string]$raw = $null) {
    $state.findings += [ordered]@{
        kind       = $kind
        severity   = $severity
        file       = $file
        line       = $line
        subsystem  = $subsystem
        evidence   = $raw
        atUtc      = (Get-Date).ToUniversalTime().ToString("o")
    }
}

function Parse-SourceSite([string]$line) {
    # Logger format: (File.cpp:123 ns::fn)
    if ($line -match '\(([^()]+\.(?:cpp|h|hpp|c)):(\d+)\s+([^)]+)\)') {
        return @{ file = $Matches[1]; line = [int]$Matches[2]; symbol = $Matches[3] }
    }
    return $null
}

function Classify-Line($state, [string]$line) {
    $site = Parse-SourceSite $line
    $frame = Get-LogFrame $line
    $state.lastInterestingLine = $line

    if ($line -match '\[LoopTrace\]') { $state.counts.loopTrace++ }
    if ($line -match '\[LoopTrace\] MUTEX') { $state.counts.mutexWait++ }
    if ($line -match 'vkWaitForFences timed out') {
        $state.counts.fenceTimeout++
        Add-Finding $state "VulkanFenceBlock" "error" `
            $(if ($site) { $site.file } else { "VulkanDeviceFrame.cpp" }) `
            $(if ($site) { $site.line } else { 222 }) `
            "VulkanRHI/SwapchainSubsystem" $line
        if ($state.classifications -notcontains "VulkanFenceBlock") {
            $state.classifications += "VulkanFenceBlock"
        }
    }
    if ($line -match 'BeginFrame failed|BeginFrame FAIL|beginFrame-FAIL') {
        $state.counts.beginFrameFail++
        Add-Finding $state "BeginFrameFail" "error" `
            $(if ($site) { $site.file } else { "RenderPipelineSubsystem.cpp" }) `
            $(if ($site) { $site.line } else { $null }) `
            "RenderPipelineSubsystem" $line
        if ($state.classifications -notcontains "BeginFrameFail") {
            $state.classifications += "BeginFrameFail"
        }
    }
    if ($line -match '(?i)assertion failed|assert\(|HE_ASSERT') {
        $state.counts.assertion++
        Add-Finding $state "AssertionFailure" "critical" `
            $(if ($site) { $site.file } else { $null }) `
            $(if ($site) { $site.line } else { $null }) `
            "Unknown" $line
        if ($state.classifications -notcontains "AssertionFailure") {
            $state.classifications += "AssertionFailure"
        }
    }
    if ($line -match 'Fatal exception|Fatal Exception|Unhandled|std::exception|SEH') {
        $state.counts.fatalException++
        Add-Finding $state "UnhandledException" "critical" `
            $(if ($site) { $site.file } else { "Main.cpp" }) `
            $(if ($site) { $site.line } else { 171 }) `
            "EditorMain" $line
        if ($state.classifications -notcontains "UnhandledException") {
            $state.classifications += "UnhandledException"
        }
    }
    if ($line -match 'focus=gained') { $state.counts.focusGained++ }
    if ($line -match 'focus=lost') { $state.counts.focusLost++ }
    if ($line -match 'Lifecycle\.WindowMinimize|Window minimized|minimized=1') { $state.counts.minimize++ }
    if ($line -match 'Lifecycle\.WindowCloseEvent|TitleBar\.CloseClick') { $state.counts.windowClose++ }
    if ($line -match 'Lifecycle\.QuitEvent|Shutdown requested|m_WantsQuit') { $state.counts.quitEvent++ }
    if ($line -match 'Swapchain recreation requested') { $state.counts.swapchainRequested++ }
    if ($line -match 'Ensuring swapchain matches') { $state.counts.swapchainEnsure++ }

    if ($line -match '\[Loop\] state=') {
        $state.lastLoopPulse = $line
        if ($line -match 'state=presented') { $state.counts.pulsePresented++ }
        if ($line -match 'beginFrame-FAIL|begin=0.*failStreak=[1-9]') { $state.counts.pulseBeginFail++ }
    }

    if ($line -match '\[LoopTrace\] ENTER Vulkan\.vkWaitForFences') {
        $state.pendingProbe = [ordered]@{
            probe      = "Vulkan.vkWaitForFences"
            file       = "Engine/Source/Runtime/VulkanRHI/Private/VulkanDeviceFrame.cpp"
            line       = 222
            subsystem  = "VulkanRHI"
            frame      = $frame
            evidence   = $line
            enteredAt  = (Get-Date)
        }
    }
    if ($line -match '\[LoopTrace\] EXIT\s+Vulkan\.vkWaitForFences') {
        $state.pendingProbe = $null
    }
    if ($line -match '\[LoopTrace\] ENTER .+PollEvents') {
        $state.pendingProbe = [ordered]@{
            probe      = "PlatformInput.PollEvents"
            file       = "Engine/Source/Programs/Editor/Private/Framework/PlatformInputSubsystem.cpp"
            line       = 77
            subsystem  = "PlatformInputSubsystem"
            frame      = $frame
            evidence   = $line
            enteredAt  = (Get-Date)
        }
    }
    if ($line -match '\[LoopTrace\] EXIT\s+.+(PollEvents|PeekMessage)') {
        if ($state.pendingProbe -and $state.pendingProbe.probe -match 'PollEvents') {
            $state.pendingProbe = $null
        }
    }
}

function Invoke-LifecycleInjection([IntPtr]$hwnd, $state, [int]$frameHint) {
    Write-Step "Injecting lifecycle at ~frame $frameHint (focus lose/gain -> minimize -> restore -> close prep)"
    $state.injectionAtFrame = $frameHint

    # Focus chatter (alt-tab style)
    [void][WeWin32Diag]::SendMessage($hwnd, [WeWin32Diag]::WM_ACTIVATE, [IntPtr][WeWin32Diag]::WA_INACTIVE, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 40
    [void][WeWin32Diag]::SetForegroundWindow($hwnd)
    [void][WeWin32Diag]::SendMessage($hwnd, [WeWin32Diag]::WM_ACTIVATE, [IntPtr][WeWin32Diag]::WA_ACTIVE, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 40

    # Minimize / restore (taskbar-style)
    [void][WeWin32Diag]::ShowWindow($hwnd, [WeWin32Diag]::SW_MINIMIZE)
    Start-Sleep -Milliseconds 120
    [void][WeWin32Diag]::ShowWindow($hwnd, [WeWin32Diag]::SW_RESTORE)
    Start-Sleep -Milliseconds 80
    [void][WeWin32Diag]::SetForegroundWindow($hwnd)

    # Rapid second focus pulse within early frames
    [void][WeWin32Diag]::SendMessage($hwnd, [WeWin32Diag]::WM_ACTIVATE, [IntPtr][WeWin32Diag]::WA_INACTIVE, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 30
    [void][WeWin32Diag]::SendMessage($hwnd, [WeWin32Diag]::WM_ACTIVATE, [IntPtr][WeWin32Diag]::WA_ACTIVE, [IntPtr]::Zero)

    $state.lifecycleInjected = $true
    Write-Step "Lifecycle injection complete"
}

function Invoke-TaskbarClose([IntPtr]$hwnd) {
    Write-Step "Posting WM_CLOSE / SC_CLOSE (taskbar close equivalent)"
    [void][WeWin32Diag]::PostMessage($hwnd, [WeWin32Diag]::WM_SYSCOMMAND, [IntPtr][WeWin32Diag]::SC_CLOSE, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 100
    [void][WeWin32Diag]::PostMessage($hwnd, [WeWin32Diag]::WM_CLOSE, [IntPtr]::Zero, [IntPtr]::Zero)
}

# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------
New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null
$state = New-TelemetryState

if (-not (Test-Path $EditorExe)) {
    throw "Editor executable not found: $EditorExe (build Development first)"
}

# Stop stale editors so session log attribution is clean
Get-Process -Name "WindeffectsEditor" -ErrorAction SilentlyContinue | ForEach-Object {
    Write-Step "Stopping stale editor pid=$($_.Id)"
    $_ | Stop-Process -Force -ErrorAction SilentlyContinue
}
Start-Sleep -Milliseconds 400

$proc = $null
$launchNotBefore = Get-Date
if (-not $SkipLaunch) {
    Write-Step "Launching editor with WE_LOOP_TRACE=1 (verbose loop telemetry)"
    $env:WE_LOOP_TRACE = "1"
    # Prefer real GPU path so fence stalls are observable; NullRHI skips the hang site.
    if (-not $env:WE_RHI) { $env:WE_RHI = "Vulkan" }

    # UseShellExecute=true avoids console log spam into this harness; env vars
    # are inherited from this process (WE_LOOP_TRACE / WE_RHI already set).
    $proc = Start-Process -FilePath $EditorExe `
        -WorkingDirectory (Split-Path $EditorExe -Parent) `
        -PassThru -WindowStyle Normal
    $state.pid = $proc.Id
    Write-Step "Editor started pid=$($proc.Id)"
} else {
    $proc = Get-Process -Name "WindeffectsEditor" -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $proc) { throw "SkipLaunch set but no WindeffectsEditor process is running" }
    $state.pid = $proc.Id
}

try {
    $logPath = Wait-NewSessionLog -notBefore $launchNotBefore -timeoutSec ([Math]::Min(45, $TimeoutSec))
    $state.logPath = $logPath
    Write-Step "Tailing session log: $logPath"

    $hwndDeadline = (Get-Date).AddSeconds(30)
    $hwnd = [IntPtr]::Zero
    while ((Get-Date) -lt $hwndDeadline) {
        $hwnd = Find-EditorHwnd -processId $state.pid
        if ($hwnd -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 150
    }
    if ($hwnd -eq [IntPtr]::Zero) {
        Add-Finding $state "WindowNotFound" "critical" "Main.cpp" 133 "Platform/Window" "EnumWindows found no WindEffects title for pid"
        throw "Editor HWND not found for pid $($state.pid)"
    }
    $state.hwnd = "0x{0:X}" -f $hwnd.ToInt64()
    Write-Step "Editor HWND=$($state.hwnd)"

    $fs = [System.IO.File]::Open($logPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    $reader = New-Object System.IO.StreamReader($fs)
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    $lastByteProgress = Get-Date
    $maxFrameSeen = -1
    $presentedFrames = 0
    $injected = $false
    $closePosted = $false
    $closeAt = $null

    while ((Get-Date) -lt $deadline) {
        $chunk = $reader.ReadToEnd()
        if ($chunk.Length -gt 0) {
            $lastByteProgress = Get-Date
            foreach ($line in ($chunk -split "`r?`n")) {
                if ([string]::IsNullOrWhiteSpace($line)) { continue }
                Classify-Line $state $line
                $fr = Get-LogFrame $line
                if ($fr -gt $maxFrameSeen) { $maxFrameSeen = $fr }
                if ($line -match '\[Loop\] state=presented') { $presentedFrames++ }

                # Inject once we have presented a few frames (target first 10).
                if (-not $injected -and ($presentedFrames -ge 2 -or ($maxFrameSeen -ge 2 -and $maxFrameSeen -le 10))) {
                    if ($maxFrameSeen -le 10 -or $presentedFrames -le 10) {
                        Invoke-LifecycleInjection -hwnd $hwnd -state $state -frameHint $maxFrameSeen
                        $injected = $true
                        $closeAt = (Get-Date).AddSeconds($KeepAliveSec)
                    }
                }
            }
        } else {
            $idle = ((Get-Date) - $lastByteProgress).TotalSeconds
            if ($idle -ge 8 -and -not $proc.HasExited) {
                $state.hangSuspected = $true
                $state.hangIdleSec = [Math]::Round($idle, 1)
                if ($state.pendingProbe) {
                    $state.hangSite = $state.pendingProbe
                } elseif (-not $state.hangSite) {
                    $site = $null
                    if ($state.lastLoopPulse) { $site = Parse-SourceSite $state.lastLoopPulse }
                    $state.hangSite = [ordered]@{
                        probe     = "LogStall"
                        file      = $(if ($site) { $site.file } else { "ApplicationFramework.cpp" })
                        line      = $(if ($site) { $site.line } else { $null })
                        subsystem = "EditorLoop"
                        frame     = $maxFrameSeen
                        evidence  = "No new log bytes for ${idle}s; lastPulse=$($state.lastLoopPulse)"
                    }
                }
                if ($state.classifications -notcontains "ThreadStall") {
                    $state.classifications += "ThreadStall"
                }
                Add-Finding $state "ThreadStall" "error" $state.hangSite.file $state.hangSite.line $state.hangSite.subsystem $state.hangSite.evidence
                Write-Step "HANG suspected: no log progress for ${idle}s"
                break
            }
        }

        if ($injected -and -not $closePosted -and $closeAt -and (Get-Date) -ge $closeAt) {
            Invoke-TaskbarClose -hwnd $hwnd
            $closePosted = $true
        }

        if ($proc.HasExited) {
            $state.exitCode = $proc.ExitCode
            Write-Step "Editor exited code=$($proc.ExitCode)"
            # Drain remaining log
            Start-Sleep -Milliseconds 300
            $tail = $reader.ReadToEnd()
            foreach ($line in ($tail -split "`r?`n")) {
                if (-not [string]::IsNullOrWhiteSpace($line)) { Classify-Line $state $line }
            }
            break
        }

        # If injection missed early frames (slow startup), force inject by wall clock once window is up
        if (-not $injected -and $maxFrameSeen -gt 10) {
            Write-Step "Startup exceeded frame 10 before present-count gate; injecting anyway at frame $maxFrameSeen"
            Invoke-LifecycleInjection -hwnd $hwnd -state $state -frameHint $maxFrameSeen
            $injected = $true
            $closeAt = (Get-Date).AddSeconds($KeepAliveSec)
        }

        Start-Sleep -Milliseconds 50
    }

    if (-not $proc.HasExited) {
        Write-Step "Timeout/cleanup - forcing editor exit"
        if (-not $closePosted -and $hwnd -ne [IntPtr]::Zero) {
            Invoke-TaskbarClose -hwnd $hwnd
            $waitExit = (Get-Date).AddSeconds(5)
            while (-not $proc.HasExited -and (Get-Date) -lt $waitExit) { Start-Sleep -Milliseconds 100 }
        }
        if (-not $proc.HasExited) {
            $proc.Kill()
            $state.exitCode = -9
            Add-Finding $state "ForcedKill" "warning" "Test-EditorLoopLifecycle.ps1" $null "Diagnostics" "Process killed after timeout"
        } else {
            $state.exitCode = $proc.ExitCode
        }
    }

    $reader.Close()
    $fs.Close()
}
finally {
    if ($proc -and -not $proc.HasExited) {
        try { $proc.Kill() } catch {}
    }
}

# Verdict - treat native crash exit codes as critical even if no log assertion fired.
$exitU = 0
if ($null -ne $state.exitCode) {
    try { $exitU = [uint32]([int64]$state.exitCode -band 0xffffffff) } catch { $exitU = 0 }
}
if ($exitU -eq 0xC0000005 -or $exitU -eq 0x80000003 -or $exitU -eq 0xC0000409) {
    $crashName = switch ($exitU) {
        0xC0000005 { "STATUS_ACCESS_VIOLATION" }
        0x80000003 { "STATUS_BREAKPOINT" }
        0xC0000409 { "STATUS_STACK_BUFFER_OVERRUN" }
        default { "NTSTATUS_0x{0:X8}" -f $exitU }
    }
    Add-Finding $state "NativeCrash" "critical" `
        "Editor shutdown after WM_CLOSE" $null "EditorMain/RHI" `
        ("exitCode={0} ({1}) - process crashed during/after taskbar close" -f $state.exitCode, $crashName)
    if ($state.classifications -notcontains "NativeCrash") {
        $state.classifications += "NativeCrash"
    }
    # Prefer last pending probe / last pulse as hang/desync site for the crash.
    if (-not $state.hangSite -and $state.pendingProbe) {
        $state.hangSite = $state.pendingProbe
    }
    if (-not $state.hangSite -and $state.lastLoopPulse) {
        $site = Parse-SourceSite $state.lastLoopPulse
        $state.hangSite = [ordered]@{
            probe     = "LastLoopPulseBeforeCrash"
            file      = $(if ($site) { $site.file } else { $null })
            line      = $(if ($site) { $site.line } else { $null })
            subsystem = "EditorLoop"
            frame     = $(Get-LogFrame $state.lastLoopPulse)
            evidence  = $state.lastLoopPulse
        }
    }
}

$critical = @($state.findings | Where-Object { $_.severity -eq "critical" }).Count
$errors = @($state.findings | Where-Object { $_.severity -eq "error" }).Count
if ($critical -gt 0) {
    $state.verdict = "FAIL_CRITICAL"
} elseif ($state.hangSuspected -or $errors -gt 0) {
    $state.verdict = "FAIL"
} elseif ($state.lifecycleInjected -and $state.counts.fenceTimeout -eq 0 -and $state.counts.assertion -eq 0 -and $state.counts.fatalException -eq 0) {
    $state.verdict = "PASS"
} else {
    $state.verdict = "INCOMPLETE"
}

$state.finishedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
$state | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $ReportPath -Encoding utf8

# Also write a short human-readable sidecar
$txtPath = [System.IO.Path]::ChangeExtension($ReportPath, ".txt")
$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine("WindEffects Editor Loop Lifecycle Diagnostic")
[void]$sb.AppendLine("Verdict: $($state.verdict)")
[void]$sb.AppendLine("Log: $($state.logPath)")
[void]$sb.AppendLine("PID: $($state.pid)  HWND: $($state.hwnd)  Exit: $($state.exitCode)")
[void]$sb.AppendLine("Injected: $($state.lifecycleInjected) at frame $($state.injectionAtFrame)")
[void]$sb.AppendLine("Classifications: $([string]::Join(', ', $state.classifications))")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("Counts:")
foreach ($k in $state.counts.Keys) {
    [void]$sb.AppendLine(("  {0,-20} {1}" -f $k, $state.counts[$k]))
}
[void]$sb.AppendLine("")
if ($state.hangSite) {
    [void]$sb.AppendLine("Hang / desync site:")
    [void]$sb.AppendLine("  probe=$($state.hangSite.probe)")
    [void]$sb.AppendLine("  file=$($state.hangSite.file):$($state.hangSite.line)")
    [void]$sb.AppendLine("  subsystem=$($state.hangSite.subsystem) frame=$($state.hangSite.frame)")
    [void]$sb.AppendLine("  evidence=$($state.hangSite.evidence)")
    [void]$sb.AppendLine("")
}
[void]$sb.AppendLine("Findings:")
foreach ($f in $state.findings) {
    [void]$sb.AppendLine(("  [{0}] {1}  {2}:{3}  ({4})" -f $f.severity, $f.kind, $f.file, $f.line, $f.subsystem))
    if ($f.evidence) { [void]$sb.AppendLine("    " + $f.evidence.Substring(0, [Math]::Min(220, $f.evidence.Length))) }
}
if ($state.lastLoopPulse) {
    [void]$sb.AppendLine("")
    [void]$sb.AppendLine("Last loop pulse:")
    [void]$sb.AppendLine("  $($state.lastLoopPulse)")
}
Set-Content -LiteralPath $txtPath -Value $sb.ToString() -Encoding utf8

Write-Host ""
Write-Host "======== TELEMETRY SUMMARY ========"
Write-Host $sb.ToString()
Write-Host "JSON report: $ReportPath"
Write-Host "Text report: $txtPath"

# Also run the existing classifier for extra LoopTrace context when a log exists
if ($state.logPath -and (Test-Path $state.logPath)) {
    Write-Host ""
    Write-Host "======== Diagnose-EditorLoopTrace ========"
    & (Join-Path $PSScriptRoot "Diagnose-EditorLoopTrace.ps1") -LogPath $state.logPath
}

if ($state.verdict -eq "PASS") { exit 0 }
if ($state.verdict -eq "INCOMPLETE") { exit 2 }
exit 1

