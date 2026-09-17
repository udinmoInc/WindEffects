// ==============================================================================
// WindEffects — IgniteBT — TemplateComplexityAnalyzer
// Source file for the IgniteBT module.
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using System.Text;
using System.Text.RegularExpressions;
using IgniteBT.Core.Database;

namespace IgniteBT.Build.Analysis;

public sealed class TemplateHotspot
{
    public string HeaderPath { get; init; } = string.Empty;
    public string HeaderName { get; init; } = string.Empty;
    public int TemplateInstantiationCount { get; init; }
    public int HeavyStlIncludesCount { get; init; }
    public int ComplexityScore => (TemplateInstantiationCount * 3) + (HeavyStlIncludesCount * 15);
    public string Recommendation { get; init; } = string.Empty;
}

public sealed class TemplateComplexityReport
{
    public List<TemplateHotspot> Hotspots { get; init; } = new();
    public int TotalHeadersAnalyzed { get; init; }
    public int HighComplexityHotspotsCount { get; init; }
}

public static class TemplateComplexityAnalyzer
{
    private static readonly HashSet<string> HeavyTemplateHeaders = new(StringComparer.OrdinalIgnoreCase)
    {
        "variant", "tuple", "functional", "map", "unordered_map", "future", "any", "type_traits"
    };

    private static readonly Regex TemplateDeclarationRegex = new(@"\btemplate\s*<[^>]+>", RegexOptions.Compiled);

    public static TemplateComplexityReport Analyze(BuildDb buildDb)
    {
        var hotspots = new List<TemplateHotspot>();
        var slowestTus = buildDb.GetTopSlowestTUs(50);
        int analyzed = 0;

        foreach (var tu in slowestTus)
        {
            if (!File.Exists(tu.SourcePath)) continue;
            analyzed++;
            var text = File.ReadAllText(tu.SourcePath);
            int templateCount = TemplateDeclarationRegex.Matches(text).Count;

            int heavyStlCount = 0;
            foreach (var stl in HeavyTemplateHeaders)
            {
                if (text.Contains($"<{stl}>") || text.Contains($"\"{stl}\""))
                    heavyStlCount++;
            }

            if (templateCount > 5 || heavyStlCount >= 2)
            {
                string rec = "Consider PImpl idiom or explicit template instantiation (extern template) to reduce compile instantiation bloat.";
                hotspots.Add(new TemplateHotspot
                {
                    HeaderPath = tu.SourcePath,
                    HeaderName = Path.GetFileName(tu.SourcePath),
                    TemplateInstantiationCount = templateCount,
                    HeavyStlIncludesCount = heavyStlCount,
                    Recommendation = rec
                });
            }
        }

        var sorted = hotspots.OrderByDescending(h => h.ComplexityScore).ToList();
        int highComplexity = sorted.Count(h => h.ComplexityScore > 30);

        return new TemplateComplexityReport
        {
            Hotspots = sorted,
            TotalHeadersAnalyzed = analyzed,
            HighComplexityHotspotsCount = highComplexity
        };
    }

    public static string FormatReport(TemplateComplexityReport report)
    {
        var sb = new StringBuilder();
        sb.AppendLine("=== Template Complexity & Instantiation Diagnostic Report ===");
        sb.AppendLine($"Analyzed Files: {report.TotalHeadersAnalyzed} | High Complexity Hotspots: {report.HighComplexityHotspotsCount}");
        sb.AppendLine();
        sb.AppendLine(string.Format("{0,-35} | {1,10} | {2,10} | {3,8} | {4}", "File / Header", "Templates", "Heavy STL", "Score", "Recommendation"));
        sb.AppendLine(new string('-', 100));

        foreach (var hs in report.Hotspots.Take(15))
        {
            sb.AppendLine(string.Format("{0,-35} | {1,10} | {2,10} | {3,8} | {4}",
                Truncate(hs.HeaderName, 35), hs.TemplateInstantiationCount, hs.HeavyStlIncludesCount, hs.ComplexityScore, hs.Recommendation));
        }

        return sb.ToString();
    }

    private static string Truncate(string str, int maxLen) =>
        str.Length <= maxLen ? str : str.Substring(0, maxLen - 3) + "...";
}
