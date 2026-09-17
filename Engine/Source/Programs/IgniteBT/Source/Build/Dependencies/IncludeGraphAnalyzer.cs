// ==============================================================================
// WindEffects — IgniteBT — IncludeGraphAnalyzer
// Source file for the IgniteBT module.
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using System.Text;
using IgniteBT.Core.Database;

namespace IgniteBT.Build.Dependencies;

public sealed class HeaderInclusionWeight
{
    public string HeaderPath { get; init; } = string.Empty;
    public string HeaderName { get; init; } = string.Empty;
    public int DirectInclusions { get; set; }
    public int TransitiveInclusions { get; set; }
    public int PchInclusions { get; set; }
    public int TotalWeight => DirectInclusions + TransitiveInclusions + PchInclusions;
}

public sealed class TuDepthInfo
{
    public string SourcePath { get; init; } = string.Empty;
    public string SourceName { get; init; } = string.Empty;
    public int DirectHeaderCount { get; init; }
    public int TransitiveHeaderCount { get; init; }
    public int MaxIncludeDepth { get; init; }
}

public sealed class IncludeGraphAnalysisReport
{
    public List<HeaderInclusionWeight> TopExpensiveHeaders { get; init; } = new();
    public List<HeaderInclusionWeight> TopMostIncludedHeaders { get; init; } = new();
    public List<TuDepthInfo> TopDeepestTUs { get; init; } = new();
    public int TotalHeadersTracked { get; init; }
    public int TotalTUsTracked { get; init; }
}

public static class IncludeGraphAnalyzer
{
    public static IncludeGraphAnalysisReport Analyze(BuildDb buildDb)
    {
        var health = buildDb.GetHealth();
        var headerWeights = new Dictionary<string, HeaderInclusionWeight>(StringComparer.OrdinalIgnoreCase);
        var tuDepths = new List<TuDepthInfo>();

        var topTus = buildDb.GetTopSlowestTUs(100);
        foreach (var tu in topTus)
        {
            if (buildDb.TryGetIncludeGraph(tu.SourcePath, out var headers))
            {
                var uniqueHeaders = new HashSet<string>(headers, StringComparer.OrdinalIgnoreCase);
                foreach (var h in uniqueHeaders)
                {
                    var hName = Path.GetFileName(h);
                    if (!headerWeights.TryGetValue(h, out var weight))
                    {
                        weight = new HeaderInclusionWeight { HeaderPath = h, HeaderName = hName };
                        headerWeights[h] = weight;
                    }
                    weight.DirectInclusions++;
                    if (tu.PchUsed) weight.PchInclusions++;
                    else weight.TransitiveInclusions++;
                }

                tuDepths.Add(new TuDepthInfo
                {
                    SourcePath = tu.SourcePath,
                    SourceName = Path.GetFileName(tu.SourcePath),
                    DirectHeaderCount = headers.Count,
                    TransitiveHeaderCount = uniqueHeaders.Count,
                    MaxIncludeDepth = Math.Min(12, Math.Max(1, headers.Count / 5))
                });
            }
        }

        var expensive = headerWeights.Values
            .OrderByDescending(w => w.TotalWeight)
            .Take(15)
            .ToList();

        var mostIncluded = headerWeights.Values
            .OrderByDescending(w => w.DirectInclusions)
            .Take(15)
            .ToList();

        var deepestTus = tuDepths
            .OrderByDescending(t => t.TransitiveHeaderCount)
            .ThenByDescending(t => t.DirectHeaderCount)
            .Take(15)
            .ToList();

        return new IncludeGraphAnalysisReport
        {
            TopExpensiveHeaders = expensive,
            TopMostIncludedHeaders = mostIncluded,
            TopDeepestTUs = deepestTus,
            TotalHeadersTracked = headerWeights.Count,
            TotalTUsTracked = tuDepths.Count
        };
    }

    public static string FormatReport(IncludeGraphAnalysisReport report)
    {
        var sb = new StringBuilder();
        sb.AppendLine("=== Include Graph Diagnostic Report ===");
        sb.AppendLine($"Headers Tracked: {report.TotalHeadersTracked} | TUs Analyzed: {report.TotalTUsTracked}");
        sb.AppendLine();
        sb.AppendLine("Top Transitive Inclusion Weights (Most Costly Headers):");
        sb.AppendLine(string.Format("{0,-35} | {1,8} | {2,8} | {3,8} | {4,8}", "Header Name", "Direct", "Trans", "PCH", "Total Wt"));
        sb.AppendLine(new string('-', 78));
        foreach (var h in report.TopExpensiveHeaders)
        {
            sb.AppendLine(string.Format("{0,-35} | {1,8} | {2,8} | {3,8} | {4,8}",
                Truncate(h.HeaderName, 35), h.DirectInclusions, h.TransitiveInclusions, h.PchInclusions, h.TotalWeight));
        }

        sb.AppendLine();
        sb.AppendLine("Top TUs by Dependency Chain Depth:");
        sb.AppendLine(string.Format("{0,-40} | {1,12} | {2,14}", "Translation Unit", "Direct Headers", "Transitive Headers"));
        sb.AppendLine(new string('-', 74));
        foreach (var tu in report.TopDeepestTUs)
        {
            sb.AppendLine(string.Format("{0,-40} | {1,12} | {2,14}",
                Truncate(tu.SourceName, 40), tu.DirectHeaderCount, tu.TransitiveHeaderCount));
        }

        return sb.ToString();
    }

    private static string Truncate(string str, int maxLen) =>
        str.Length <= maxLen ? str : str.Substring(0, maxLen - 3) + "...";
}
