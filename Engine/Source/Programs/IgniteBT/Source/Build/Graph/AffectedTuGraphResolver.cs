// ==============================================================================
// WindEffects — IgniteBT — AffectedTuGraphResolver
// Source file for the IgniteBT module.
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.Build.Dependencies;
using IgniteBT.Core.Database;
using IgniteBT.Workspace.Modules;
using Serilog;

namespace IgniteBT.Build.Graph;

public sealed class AffectedTuResolutionResult
{
    public HashSet<string> AffectedSourcePaths { get; } = new(StringComparer.OrdinalIgnoreCase);
    public HashSet<string> InvalidatedModules { get; } = new(StringComparer.OrdinalIgnoreCase);
    public bool RequiresPchRebuild { get; set; }
    public bool RebuildAll { get; set; }
    public string InvalidationReason { get; set; } = string.Empty;
}

public static class AffectedTuGraphResolver
{
    public static AffectedTuResolutionResult ResolveAffectedTus(
        IEnumerable<string> modifiedFiles,
        DependencyGraph graph,
        SqliteBuildDatabase buildDb,
        List<DiscoveredModule> modules)
    {
        var result = new AffectedTuResolutionResult();
        var moduleMap = modules.ToDictionary(m => m.Name, StringComparer.OrdinalIgnoreCase);

        foreach (var file in modifiedFiles)
        {
            var module = modules.FirstOrDefault(m => file.StartsWith(m.ModuleDirectory, StringComparison.OrdinalIgnoreCase));
            var detail = ExactChangeClassifier.Classify(file, module, graph);

            switch (detail.Classification)
            {
                case ChangeClassification.CPP_IMPLEMENTATION:
                    result.AffectedSourcePaths.Add(file);
                    Log.Debug("Invalidation target: TU {File}", Path.GetFileName(file));
                    break;

                case ChangeClassification.PRIVATE_HEADER:
                    var privateDeps = buildDb.GetTUsIncludingHeader(file);
                    if (privateDeps.Count > 0)
                    {
                        foreach (var dep in privateDeps)
                        {
                            result.AffectedSourcePaths.Add(dep);
                        }
                        Log.Debug("Invalidation target: Private Header {Header} -> {Count} dependent TUs", Path.GetFileName(file), privateDeps.Count);
                    }
                    else if (module != null)
                    {
                        // Fallback: invalidate all sources in module if graph is missing
                        foreach (var src in module.SourceFiles)
                        {
                            result.AffectedSourcePaths.Add(src);
                        }
                    }
                    break;

                case ChangeClassification.PUBLIC_HEADER:
                    var publicDeps = buildDb.GetTransitiveIncludeTUs(file);
                    if (publicDeps.Count > 0)
                    {
                        foreach (var dep in publicDeps)
                        {
                            result.AffectedSourcePaths.Add(dep);
                        }
                        Log.Debug("Invalidation target: Public Header {Header} -> {Count} transitive TUs", Path.GetFileName(file), publicDeps.Count);
                    }
                    else if (module != null)
                    {
                        foreach (var src in module.SourceFiles)
                        {
                            result.AffectedSourcePaths.Add(src);
                        }
                    }
                    break;

                case ChangeClassification.PCH_HEADER:
                    if (module != null)
                    {
                        result.InvalidatedModules.Add(module.Name);
                        result.RequiresPchRebuild = true;
                        foreach (var src in module.SourceFiles)
                        {
                            result.AffectedSourcePaths.Add(src);
                        }
                    }
                    break;

                case ChangeClassification.MODULE_DEFINITION:
                    if (module != null)
                    {
                        result.InvalidatedModules.Add(module.Name);
                        foreach (var src in module.SourceFiles)
                        {
                            result.AffectedSourcePaths.Add(src);
                        }
                    }
                    break;

                case ChangeClassification.BUILD_CONFIGURATION:
                case ChangeClassification.COMPILER_CONFIGURATION:
                    result.RebuildAll = true;
                    result.InvalidationReason = $"Global change ({detail.Classification})";
                    return result;

                case ChangeClassification.ASSET_ONLY:
                case ChangeClassification.UNRELATED:
                case ChangeClassification.GENERATED_HEADER:
                    // Asset or non-code changes do not invalidate C++ translation units
                    break;
            }
        }

        result.InvalidationReason = $"Fine-grained TU resolution: {result.AffectedSourcePaths.Count} TUs affected across {result.InvalidatedModules.Count} modules.";
        return result;
    }
}
