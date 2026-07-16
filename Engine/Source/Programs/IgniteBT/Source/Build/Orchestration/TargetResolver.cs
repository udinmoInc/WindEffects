using IgniteBT.Build.Graph;
using IgniteBT.Workspace.Modules;
using Serilog;

namespace IgniteBT.Build.Orchestration;

/// <summary>
/// Resolves --target NAME to the minimal module set required for that target.
/// </summary>
public static class TargetResolver
{
  private static readonly Dictionary<string, TargetSpec> KnownTargets = new(StringComparer.OrdinalIgnoreCase)
    {
        ["Editor"] = new TargetSpec
        {
            RootModules = ["WindeffectsEditor", "Application", "Editor"],
            IncludeDependents = false
        },
        ["Engine"] = new TargetSpec
        {
            RootModules = ["WindeffectsEngine", "Engine", "Core"],
            IncludeDependents = false
        },
        ["Renderer"] = new TargetSpec
        {
            RootModules = ["Renderer", "RHI", "VulkanRHI"],
            IncludeDependents = false
        },
        ["Game"] = new TargetSpec
        {
            RootModules = ["WindeffectsGame", "Game"],
            IncludeDependents = false
        },
        ["WeLauncher"] = new TargetSpec
        {
            RootModules = ["WeLauncher"],
            IncludeDependents = false
        }
    };

    public static List<DiscoveredModule> FilterModules(
        List<DiscoveredModule> allModules,
        DependencyGraph graph,
        string? targetName,
        HashSet<string>? unityDisabledModules = null)
    {
        if (string.IsNullOrWhiteSpace(targetName))
            return allModules;

        // Direct module name target
        if (allModules.Any(m => m.Name.Equals(targetName, StringComparison.OrdinalIgnoreCase)))
        {
            var required = CollectTransitiveDependencies(graph, targetName);
            Log.Information("Target '{Target}': building {Count}/{Total} modules",
                targetName, required.Count, allModules.Count);
            return allModules.Where(m => required.Contains(m.Name)).ToList();
        }

        if (!KnownTargets.TryGetValue(targetName, out var spec))
        {
            Log.Warning("Unknown target '{Target}', building all modules", targetName);
            return allModules;
        }

        var roots = spec.RootModules
            .SelectMany(r => allModules.Where(m => m.Name.Contains(r, StringComparison.OrdinalIgnoreCase)).Select(m => m.Name))
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToList();

        if (roots.Count == 0)
        {
            Log.Warning("Target '{Target}' matched no root modules", targetName);
            return allModules;
        }

        var moduleSet = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var root in roots)
            moduleSet.UnionWith(CollectTransitiveDependencies(graph, root));

        if (unityDisabledModules != null)
            foreach (var m in unityDisabledModules) moduleSet.Add(m);

        Log.Information("Target '{Target}': building {Count}/{Total} modules",
            targetName, moduleSet.Count, allModules.Count);

        return allModules.Where(m => moduleSet.Contains(m.Name)).ToList();
    }

    private static HashSet<string> CollectTransitiveDependencies(DependencyGraph graph, string rootModule)
    {
        var result = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var node = graph.GetNode(rootModule);
        if (node == null) { result.Add(rootModule); return result; }

        CollectDeps(node, result);
        result.Add(rootModule);
        return result;
    }

    private static void CollectDeps(BuildNode node, HashSet<string> result)
    {
        foreach (var dep in node.Dependencies)
        {
            if (result.Add(dep.Name))
                CollectDeps(dep, result);
        }
    }

    private sealed class TargetSpec
    {
        public List<string> RootModules { get; init; } = new();
        public bool IncludeDependents { get; init; }
    }
}
