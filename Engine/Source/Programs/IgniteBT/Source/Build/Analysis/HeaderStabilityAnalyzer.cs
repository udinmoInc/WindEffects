// ==============================================================================
// WindEffects — IgniteBT — HeaderStabilityAnalyzer
// Source file for the IgniteBT module.
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using System.Text;
using IgniteBT.Core.Database;

namespace IgniteBT.Build.Analysis;

public enum HeaderStabilityCategory
{
    HOT,
    WARM,
    STABLE
}

public sealed class HeaderStabilityItem
{
    public string HeaderName { get; init; } = string.Empty;
    public string HeaderPath { get; init; } = string.Empty;
    public HeaderStabilityCategory Category { get; init; }
    public int DependentTuCount { get; init; }
    public string Recommendation { get; init; } = string.Empty;
}

public static class HeaderStabilityAnalyzer
{
    public static List<HeaderStabilityItem> Analyze(BuildDb buildDb)
    {
        var items = new List<HeaderStabilityItem>();
        var topTus = buildDb.GetTopSlowestTUs(100);
        var headerCounts = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);

        foreach (var tu in topTus)
        {
            if (buildDb.TryGetIncludeGraph(tu.SourcePath, out var headers))
            {
                foreach (var h in headers.Distinct(StringComparer.OrdinalIgnoreCase))
                {
                    headerCounts[h] = headerCounts.GetValueOrDefault(h, 0) + 1;
                }
            }
        }

        foreach (var (h, count) in headerCounts.OrderByDescending(kv => kv.Value).Take(25))
        {
            var hName = Path.GetFileName(h);
            var category = HeaderStabilityCategory.STABLE;
            string rec = "Safe for PCH inclusion.";

            if (hName.Contains("Editor") || hName.Contains("UI") || hName.Contains("Panel"))
            {
                category = HeaderStabilityCategory.HOT;
                rec = "Do NOT put in PCH. Frequently modified UI/Editor header.";
            }
            else if (hName.Contains("Subsystem") || hName.Contains("Processor"))
            {
                category = HeaderStabilityCategory.WARM;
                rec = "Include conditionally or via direct header include.";
            }

            items.Add(new HeaderStabilityItem
            {
                HeaderName = hName,
                HeaderPath = h,
                Category = category,
                DependentTuCount = count,
                Recommendation = rec
            });
        }

        return items;
    }

    public static string FormatReport(BuildDb buildDb)
    {
        var items = Analyze(buildDb);
        if (items.Count == 0) return string.Empty;

        var sb = new StringBuilder();
        sb.AppendLine("=== Header Stability Analysis Report ===");
        sb.AppendLine(string.Format("{0,-30} | {1,-8} | {2,10} | {3}", "Header Name", "Category", "TUs Count", "PCH Recommendation"));
        sb.AppendLine(new string('-', 85));

        foreach (var item in items.Take(15))
        {
            sb.AppendLine(string.Format("{0,-30} | {1,-8} | {2,10} | {3}",
                Truncate(item.HeaderName, 30), item.Category, item.DependentTuCount, item.Recommendation));
        }

        return sb.ToString();
    }

    private static string Truncate(string str, int maxLen) =>
        str.Length <= maxLen ? str : str.Substring(0, maxLen - 3) + "...";
}
