// ==============================================================================
// WindEffects — IgniteBT — BuildProfilerTelemetry
// Diagnostic telemetry provider wrapping BuildProfiler for debug/profiling modes.
// Maintained and authored by Vijay Singh and John Anderson.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using Serilog;
using IgniteBT.Build.Analysis;
using IgniteBT.Core.Profiling;

namespace IgniteBT.Diagnostics.Telemetry;

/// <summary>
/// Telemetry implementation for Debug / Diagnostic mode that records fine-grained performance profiles.
/// </summary>
public sealed class BuildProfilerTelemetry : IBuildTelemetry
{
    private readonly BuildProfiler _profiler;
    private readonly bool _verbose;

    public BuildProfiler TelemetryProfiler => _profiler;

    public BuildProfilerTelemetry(BuildProfiler profiler, bool verbose = true)
    {
        _profiler = profiler ?? new BuildProfiler();
        _verbose = verbose;
    }

    public void OnBuildStarted(string targetName, int totalTus)
    {
        if (_verbose)
        {
            Log.Information("[Debug] Building {Target} ({Count} TUs)...", targetName, totalTus);
        }
    }

    public void OnBuildFinished(bool success, double totalMs, bool wasNoOp)
    {
        if (_verbose)
        {
            Log.Information("[Debug] Build finished. Success: {Success}, NoOp: {NoOp}, Time: {Time:F2}ms",
                success, wasNoOp, totalMs);
        }
    }

    public void OnCompilationStarted(string fileBasename, int current, int total)
    {
        if (_verbose)
        {
            Log.Information("[Debug] [{Current}/{Total}] Compiling {File}", current, total, fileBasename);
        }
    }

    public void OnCompilationFinished(string fileBasename, bool success, double elapsedMs)
    {
        if (_verbose)
        {
            Log.Information("[Debug] Compiled {File} in {Time:F1}ms (Success: {Success})", fileBasename, elapsedMs, success);
        }
    }

    public void OnLinkingStarted(string targetName)
    {
        if (_verbose)
        {
            Log.Information("[Debug] Linking target {Target}...", targetName);
        }
    }

    public void OnLinkingFinished(string targetName, bool success, double elapsedMs)
    {
        if (_verbose)
        {
            Log.Information("[Debug] Linked {Target} in {Time:F1}ms (Success: {Success})", targetName, elapsedMs, success);
        }
    }

    public void OnDiagnosticEvent(string category, string message)
    {
        if (_verbose)
        {
            Log.Information("[Debug:{Category}] {Message}", category, message);
        }
    }
}
