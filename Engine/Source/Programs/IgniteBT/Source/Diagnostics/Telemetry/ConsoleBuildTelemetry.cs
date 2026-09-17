// ==============================================================================
// WindEffects — IgniteBT — ConsoleBuildTelemetry
// Clean production progress telemetry with animated console progress for interactive builds.
// Maintained and authored by Vijay Singh and John Anderson.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using System.Text;
using Serilog;

namespace IgniteBT.Diagnostics.Telemetry;

/// <summary>
/// Clean progress reporter for Normal Mode builds with animated console progress bar.
/// </summary>
public sealed class ConsoleBuildTelemetry : IBuildTelemetry
{
    private static readonly string[] SpinnerFrames = ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"];
    private int _spinnerIndex;
    private int _completedCount;
    private int _totalTus;
    private string _targetName = "WindEffects";
    private readonly object _lock = new();

    public ConsoleBuildTelemetry()
    {
        try
        {
            Console.OutputEncoding = Encoding.UTF8;
        }
        catch { }
    }

    public void OnBuildStarted(string targetName, int totalTus)
    {
        _targetName = targetName;
        _totalTus = totalTus;
        _completedCount = 0;
        Log.Information("Building {Target} ({Count} translation units)...", targetName, totalTus);
    }

    public void OnBuildFinished(bool success, double totalMs, bool wasNoOp)
    {
        lock (_lock)
        {
            if (!Console.IsOutputRedirected && Console.CursorLeft > 0)
            {
                Console.WriteLine();
            }
        }

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
        RenderProgress(fileBasename, current, total, "Compiling");
    }

    public void OnCompilationFinished(string fileBasename, bool success, double elapsedMs)
    {
        Interlocked.Increment(ref _completedCount);
        if (!success)
        {
            Log.Error("Failed compiling {File}", fileBasename);
        }
    }

    public void OnLinkingStarted(string targetName)
    {
        RenderProgress(targetName, _completedCount, _totalTus > 0 ? _totalTus : 1, "Linking");
    }

    public void OnLinkingFinished(string targetName, bool success, double elapsedMs)
    {
        if (!success)
        {
            Log.Error("Failed linking {Target}", targetName);
        }
    }

    public void OnDiagnosticEvent(string category, string message) { }

    private void RenderProgress(string item, int current, int total, string action)
    {
        if (Console.IsOutputRedirected)
        {
            Log.Information("[{Current}/{Total}] {Action} {Item}", current, total, action, item);
            return;
        }

        lock (_lock)
        {
            int spinnerIdx = Interlocked.Increment(ref _spinnerIndex) % SpinnerFrames.Length;
            string spinner = SpinnerFrames[spinnerIdx];
            double pct = total > 0 ? (double)current / total : 0.0;
            int barWidth = 20;
            int filled = Math.Clamp((int)(pct * barWidth), 0, barWidth);
            string bar = new string('█', filled) + new string('░', barWidth - filled);

            string line = $"\r {spinner} [{bar}] {pct:P0} [{current}/{total}] {action} {item}";
            int width = Math.Max(0, Console.WindowWidth - 1);
            if (width > 0 && line.Length > width)
            {
                line = line[..width];
            }
            Console.Write(line.PadRight(width > 0 ? width : line.Length));
        }
    }
}
