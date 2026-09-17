// ==============================================================================
// WindEffects — IgniteBT — IwyuAnalyzer
// Source file for the IgniteBT module.
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using System.Text;
using System.Text.RegularExpressions;
using IgniteBT.Core.Database;

namespace IgniteBT.Build.Analysis;

public enum ImpactSeverity
{
    LOW,
    MEDIUM,
    HIGH
}

public sealed class IwyuRecommendation
{
    public string TargetFile { get; init; } = string.Empty;
    public string TargetFileName { get; init; } = string.Empty;
    public string RuleName { get; init; } = string.Empty;
    public string Finding { get; init; } = string.Empty;
    public string Recommendation { get; init; } = string.Empty;
    public ImpactSeverity Severity { get; init; }
    public int EstimatedSavingsMs { get; init; }
}

public sealed class IwyuAnalysisReport
{
    public List<IwyuRecommendation> Recommendations { get; init; } = new();
    public int TotalFilesAudited { get; init; }
    public int TotalHighImpactIssues { get; init; }
}

public static class IwyuAnalyzer
{
    private static readonly HashSet<string> HeavyUmbrellaHeaders = new(StringComparer.OrdinalIgnoreCase)
    {
        "windows.h", "Compilation.h", "CoreSDK.h", "Engine.h", "UnrealEngine.h"
    };

    private static readonly Regex IncludeRegex = new(@"^\s*#\s*include\s*[<""](?<header>[^"">]+)[>""]", RegexOptions.Compiled);
    private static readonly Regex PointerParamRegex = new(@"\b(class|struct)\s+(?<type>\w+)\s*\*", RegexOptions.Compiled);

    public static IwyuAnalysisReport Analyze(BuildDb buildDb)
    {
        var recommendations = new List<IwyuRecommendation>();
        var slowestTus = buildDb.GetTopSlowestTUs(50);
        int audited = 0;

        foreach (var tu in slowestTus)
        {
            if (!File.Exists(tu.SourcePath)) continue;
            audited++;
            var lines = File.ReadAllLines(tu.SourcePath);
            var seenHeaders = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

            for (int i = 0; i < lines.Length; i++)
            {
                var line = lines[i];
                var match = IncludeRegex.Match(line);
                if (match.Success)
                {
                    var header = match.Groups["header"].Value;
                    var headerName = Path.GetFileName(header);

                    // 1. Heavy umbrella header detection
                    if (HeavyUmbrellaHeaders.Contains(headerName))
                    {
                        recommendations.Add(new IwyuRecommendation
                        {
                            TargetFile = tu.SourcePath,
                            TargetFileName = Path.GetFileName(tu.SourcePath),
                            RuleName = "Umbrella Header Include",
                            Finding = $"Includes umbrella header '{header}' at line {i + 1}",
                            Recommendation = $"Replace '{header}' with fine-grained modular headers to eliminate macro & header pollution.",
                            Severity = ImpactSeverity.HIGH,
                            EstimatedSavingsMs = 400
                        });
                    }

                    // 2. Duplicate header inclusion check
                    if (!seenHeaders.Add(header))
                    {
                        recommendations.Add(new IwyuRecommendation
                        {
                            TargetFile = tu.SourcePath,
                            TargetFileName = Path.GetFileName(tu.SourcePath),
                            RuleName = "Duplicate Header Include",
                            Finding = $"Redundant #include '{header}' at line {i + 1}",
                            Recommendation = $"Remove duplicate #include directive.",
                            Severity = ImpactSeverity.LOW,
                            EstimatedSavingsMs = 20
                        });
                    }
                }
            }
        }

        var highImpactCount = recommendations.Count(r => r.Severity == ImpactSeverity.HIGH);

        return new IwyuAnalysisReport
        {
            Recommendations = recommendations.OrderByDescending(r => r.Severity).ThenByDescending(r => r.EstimatedSavingsMs).ToList(),
            TotalFilesAudited = audited,
            TotalHighImpactIssues = highImpactCount
        };
    }

    public static string FormatReport(IwyuAnalysisReport report)
    {
        var sb = new StringBuilder();
        sb.AppendLine("=== IWYU Structural Diagnostic Report ===");
        sb.AppendLine($"Audited Files: {report.TotalFilesAudited} | High Impact Findings: {report.TotalHighImpactIssues}");
        sb.AppendLine();
        sb.AppendLine(string.Format("{0,-8} | {1,-30} | {2,-25} | {3}", "Severity", "File", "Rule", "Recommendation"));
        sb.AppendLine(new string('-', 100));

        foreach (var rec in report.Recommendations.Take(20))
        {
            sb.AppendLine(string.Format("{0,-8} | {1,-30} | {2,-25} | {3}",
                rec.Severity, Truncate(rec.TargetFileName, 30), Truncate(rec.RuleName, 25), rec.Recommendation));
        }

        return sb.ToString();
    }

    private static string Truncate(string str, int maxLen) =>
        str.Length <= maxLen ? str : str.Substring(0, maxLen - 3) + "...";
}
