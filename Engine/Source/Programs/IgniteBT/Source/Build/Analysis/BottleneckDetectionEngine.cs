// ==============================================================================
// WindEffects — IgniteBT — BottleneckDetectionEngine
// Source file for the IgniteBT module.
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using System.Text;
using IgniteBT.Core.Profiling;

namespace IgniteBT.Build.Analysis;

public enum BottleneckSeverity
{
    MeasuredBottleneck,
    SecondaryBottleneck,
    MinorCost,
    NotSignificant
}

public sealed class PipelineBottleneckItem
{
    public string ComponentName { get; init; } = string.Empty;
    public long MeasuredCostMs { get; init; }
    public double PercentageShare { get; init; }
    public BottleneckSeverity Severity { get; init; }
    public string Evidence { get; init; } = string.Empty;
    public string PossibleCause { get; init; } = string.Empty;
    public string RecommendedOptimization { get; init; } = string.Empty;
    public string Confidence { get; init; } = "HIGH";
}

public static class BottleneckDetectionEngine
{
    public static List<PipelineBottleneckItem> DetectBottlenecks(BuildProfileReport profile)
    {
        var items = new List<PipelineBottleneckItem>();
        long totalMs = Math.Max(1, profile.TotalBuildMs);

        foreach (var scope in profile.Scopes)
        {
            if (scope.TotalMs <= 0) continue;
            double share = ((double)scope.TotalMs / totalMs) * 100.0;

            var severity = BottleneckSeverity.NotSignificant;
            if (share >= 40.0) severity = BottleneckSeverity.MeasuredBottleneck;
            else if (share >= 15.0) severity = BottleneckSeverity.SecondaryBottleneck;
            else if (share >= 5.0) severity = BottleneckSeverity.MinorCost;

            string cause = "Stage cost proportional to active workload.";
            string rec = "Monitor stage scaling.";
            string confidence = "HIGH";

            switch (scope.Name)
            {
                case BuildStages.Compile:
                    cause = "MSVC C++ preprocessor parsing, template expansion, and code generation.";
                    rec = "Decouple heavy headers (Logger.h, <sstream>), expand module PCH, and use forward declarations.";
                    break;
                case BuildStages.HeaderScan:
                    cause = "Preprocessor dependency scanning and header graph resolution.";
                    rec = "Reuse SQLite header dependency graph and (path, size, mtime) fast path cache.";
                    break;
                case BuildStages.Link:
                    cause = "Linker MSVCLinker object loading and PDB symbol processing.";
                    rec = "Use incremental linking and restrict link invalidation to affected targets.";
                    break;
                case BuildStages.ModuleDiscovery:
                    cause = "Directory enumeration and Module.cs build script parsing.";
                    rec = "Reuse cached module discovery manifest.";
                    break;
            }

            items.Add(new PipelineBottleneckItem
            {
                ComponentName = scope.Name,
                MeasuredCostMs = scope.TotalMs,
                PercentageShare = share,
                Severity = severity,
                Evidence = $"Measured execution: {scope.TotalMs} ms across {scope.Count} invocation(s)",
                PossibleCause = cause,
                RecommendedOptimization = rec,
                Confidence = confidence
            });
        }

        return items.OrderByDescending(i => i.MeasuredCostMs).ToList();
    }

    public static string FormatReport(BuildProfileReport profile)
    {
        var items = DetectBottlenecks(profile);
        if (items.Count == 0) return string.Empty;

        var sb = new StringBuilder();
        sb.AppendLine("=======================================================================================");
        sb.AppendLine("IgniteBT Automatic Pipeline Bottleneck Report");
        sb.AppendLine("=======================================================================================");

        int rank = 1;
        foreach (var item in items.Where(i => i.Severity != BottleneckSeverity.NotSignificant).Take(5))
        {
            sb.AppendLine($"BOTTLENECK #{rank++} — [{item.Severity}]");
            sb.AppendLine($"Component:      {item.ComponentName}");
            sb.AppendLine($"Cost:           {item.MeasuredCostMs} ms ({item.PercentageShare:F1}% share)");
            sb.AppendLine($"Evidence:       {item.Evidence}");
            sb.AppendLine($"Possible Cause: {item.PossibleCause}");
            sb.AppendLine($"Recommendation: {item.RecommendedOptimization}");
            sb.AppendLine($"Confidence:     {item.Confidence}");
            sb.AppendLine(new string('-', 87));
        }

        return sb.ToString();
    }
}
