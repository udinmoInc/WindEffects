// ==============================================================================
// WindEffects — IgniteBT — ConsoleBuildTelemetry
// Clean production progress telemetry for Normal Mode.
// Maintained and authored by Vijay Singh and John Anderson.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using Serilog;

namespace IgniteBT.Diagnostics.Telemetry;

/// <summary>
/// Clean progress reporter for Normal Mode builds.
/// </summary>
public sealed class ConsoleBuildTelemetry : IBuildTelemetry
{
    public void OnBuildStarted(string targetName, int totalTus)
    {
        Log.Information("Building {Target} ({Count} translation units)...", targetName, totalTus);
    }

    public void OnBuildFinished(bool success, double totalMs, bool wasNoOp)
    {
        if (wasNoOp)
        {
            Log.Information("Build up-to-date ({Time:F0} ms)", totalMs);
        }
        else if (success)
        {
            Log.Information("Build succeeded in {Time:F2}s", totalMs / 1000.0);
        }
        else
        {
            Log.Error("Build failed after {Time:F2}s", totalMs / 1000.0);
        }
    }

    public void OnCompilationStarted(string fileBasename, int current, int total)
    {
        Log.Information("[{Current}/{Total}] {File}", current, total, fileBasename);
    }

    public void OnCompilationFinished(string fileBasename, bool success, double elapsedMs)
    {
        if (!success)
        {
            Log.Error("Failed compiling {File}", fileBasename);
        }
    }

    public void OnLinkingStarted(string targetName)
    {
        Log.Information("Linking {Target}...", targetName);
    }

    public void OnLinkingFinished(string targetName, bool success, double elapsedMs)
    {
        if (!success)
        {
            Log.Error("Failed linking {Target}", targetName);
        }
    }

    public void OnDiagnosticEvent(string category, string message)
    {
        // Suppressed in Normal Mode
    }
}
