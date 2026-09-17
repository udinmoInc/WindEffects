// ==============================================================================
// WindEffects — IgniteBT — PchEffectivenessAnalyzer
// Source file for the IgniteBT module.
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using System.Text;
using IgniteBT.Core.Database;

namespace IgniteBT.Build.Analysis;

public sealed class ModulePchAudit
{
    public string ModuleName { get; init; } = string.Empty;
    public string PchHeaderName { get; init; } = string.Empty;
    public int TotalTUs { get; init; }
    public int TUsUsingPch { get; init; }
    public double HitRatioPercent => TotalTUs > 0 ? ((double)TUsUsingPch / TotalTUs) * 100.0 : 0;
    public List<string> MissingFoundationHeaders { get; init; } = new();
    public List<string> VolatileHeadersInPch { get; init; } = new();
    public string Status { get; init; } = "OPTIMAL";
}

public sealed class PchEffectivenessReport
{
    public List<ModulePchAudit> ModuleAudits { get; init; } = new();
    public double AveragePchHitRatioPercent { get; init; }
    public int UnderpoweredModulesCount { get; init; }
    public int OversizedModulesCount { get; init; }
}

public static class PchEffectivenessAnalyzer
{
    private static readonly HashSet<string> FoundationHeaders = new(StringComparer.OrdinalIgnoreCase)
    {
        "vector", "string", "memory", "unordered_map", "utility", "algorithm", "cstdint", "functional", "type_traits"
    };

    public static PchEffectivenessReport Analyze(BuildDb buildDb)
    {
        var audits = new List<ModulePchAudit>();
        var slowestTus = buildDb.GetTopSlowestTUs(100);
        var tuGroups = slowestTus.GroupBy(t => t.ModuleName, StringComparer.OrdinalIgnoreCase);

        foreach (var group in tuGroups)
        {
            var moduleName = group.Key;
            var tus = group.ToList();
            var pchTus = tus.Where(t => t.PchUsed).ToList();
            var pchName = pchTus.FirstOrDefault()?.PchName ?? $"{moduleName}PrivatePCH.h";

            var missingFoundation = new List<string>();
            var volatileHeaders = new List<string>();

            // Audit missing foundation headers across TUs in module
            var nonPchTus = tus.Where(t => !t.PchUsed).ToList();
            if (nonPchTus.Count > 0)
            {
                foreach (var fh in FoundationHeaders)
                {
                    if (nonPchTus.Any(t => t.HeaderCount > 10))
                    {
                        missingFoundation.Add($"<{fh}>");
                    }
                }
            }

            string status = "OPTIMAL";
            if (pchTus.Count == 0 && tus.Count > 2)
            {
                status = "UNDERPOWERED (PCH Disabled/Unused)";
            }
            else if (missingFoundation.Count > 3)
            {
                status = "UNDERPOWERED (Missing STL Foundation)";
            }

            audits.Add(new ModulePchAudit
            {
                ModuleName = moduleName,
                PchHeaderName = pchName,
                TotalTUs = tus.Count,
                TUsUsingPch = pchTus.Count,
                MissingFoundationHeaders = missingFoundation.Distinct().Take(4).ToList(),
                VolatileHeadersInPch = volatileHeaders,
                Status = status
            });
        }

        double avgHit = audits.Count > 0 ? audits.Average(a => a.HitRatioPercent) : 100.0;
        int underpowered = audits.Count(a => a.Status.StartsWith("UNDERPOWERED"));
        int oversized = audits.Count(a => a.Status.StartsWith("OVERSIZED"));

        return new PchEffectivenessReport
        {
            ModuleAudits = audits.OrderByDescending(a => a.TotalTUs).ToList(),
            AveragePchHitRatioPercent = avgHit,
            UnderpoweredModulesCount = underpowered,
            OversizedModulesCount = oversized
        };
    }

    public static string FormatReport(PchEffectivenessReport report)
    {
        var sb = new StringBuilder();
        sb.AppendLine("=== PCH Effectiveness Diagnostic Audit ===");
        sb.AppendLine($"Average Hit Ratio: {report.AveragePchHitRatioPercent:F1}% | Underpowered: {report.UnderpoweredModulesCount} | Oversized: {report.OversizedModulesCount}");
        sb.AppendLine();
        sb.AppendLine(string.Format("{0,-25} | {1,8} | {2,8} | {3,10} | {4}", "Module", "Total TUs", "PCH TUs", "Hit Rate", "Audit Status"));
        sb.AppendLine(new string('-', 85));

        foreach (var audit in report.ModuleAudits.Take(15))
        {
            sb.AppendLine(string.Format("{0,-25} | {1,8} | {2,8} | {3,9:F1}% | {4}",
                Truncate(audit.ModuleName, 25), audit.TotalTUs, audit.TUsUsingPch, audit.HitRatioPercent, audit.Status));
        }

        return sb.ToString();
    }

    private static string Truncate(string str, int maxLen) =>
        str.Length <= maxLen ? str : str.Substring(0, maxLen - 3) + "...";
}
