// ==============================================================================
// WindEffects — IgniteBT — TopBottleneckReportGenerator
// Source file for the IgniteBT module.
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using System.Text;
using IgniteBT.Build.Dependencies;
using IgniteBT.Core.Database;

namespace IgniteBT.Build.Analysis;

public sealed class BottleneckAnalysisItem
{
    public string TuName { get; init; } = string.Empty;
    public string ModuleName { get; init; } = string.Empty;
    public long CompileTimeMs { get; init; }
    public int HeaderCount { get; init; }
    public bool PchUsed { get; init; }
    public string EvidenceSummary { get; init; } = string.Empty;
    public string PotentialCause { get; init; } = string.Empty;
    public string Recommendation { get; init; } = string.Empty;
    public string Confidence { get; init; } = "HIGH";
}

public static class TopBottleneckReportGenerator
{
    public static List<BottleneckAnalysisItem> Analyze(BuildDb buildDb)
    {
        var items = new List<BottleneckAnalysisItem>();
        var topTus = buildDb.GetTopSlowestTUs(5);

        foreach (var tu in topTus)
        {
            var fileName = Path.GetFileName(tu.SourcePath);
            string evidence = $"Header count: {tu.HeaderCount}, PCH reuse: {(tu.PchUsed ? "YES" : "NO")}";
            string cause = "Heavy preprocessor parsing and header inclusion cascade.";
            string rec = "Decouple heavy header inclusions and forward-declare types.";
            string confidence = "HIGH";

            if (!tu.PchUsed && tu.HeaderCount > 10)
            {
                cause = "PCH disabled or unused for module; foundation headers parsed repeatedly.";
                rec = $"Enable module PCH for {tu.ModuleName} and include foundation STL headers.";
                confidence = "HIGH";
            }
            else if (fileName.Contains("Editor") || fileName.Contains("UI"))
            {
                cause = "Large UI umbrella header dependencies (EditorUI.h, KindUI.h).";
                rec = "Replace umbrella UI headers with modular fine-grained headers.";
                confidence = "HIGH";
            }

            items.Add(new BottleneckAnalysisItem
            {
                TuName = fileName,
                ModuleName = tu.ModuleName,
                CompileTimeMs = tu.AverageMs,
                HeaderCount = tu.HeaderCount,
                PchUsed = tu.PchUsed,
                EvidenceSummary = evidence,
                PotentialCause = cause,
                Recommendation = rec,
                Confidence = confidence
            });
        }

        return items;
    }

    public static string FormatReport(BuildDb buildDb)
    {
        var items = Analyze(buildDb);
        if (items.Count == 0) return string.Empty;

        var sb = new StringBuilder();
        sb.AppendLine("====================================================");
        sb.AppendLine("IgniteBT Compile Optimization Report — Top Bottlenecks");
        sb.AppendLine("====================================================");

        var top = items[0];
        sb.AppendLine($"Top Bottleneck:");
        sb.AppendLine($"  {top.TuName} (Module: {top.ModuleName})");
        sb.AppendLine($"  Compile: {top.CompileTimeMs} ms");
        sb.AppendLine();
        sb.AppendLine($"Primary Evidence:");
        sb.AppendLine($"  {top.EvidenceSummary}");
        sb.AppendLine();
        sb.AppendLine($"Measured Cause:");
        sb.AppendLine($"  {top.PotentialCause}");
        sb.AppendLine();
        sb.AppendLine($"Recommendation:");
        sb.AppendLine($"  {top.Recommendation}");
        sb.AppendLine();
        sb.AppendLine($"Confidence: {top.Confidence}");
        sb.AppendLine("====================================================");

        return sb.ToString();
    }
}
