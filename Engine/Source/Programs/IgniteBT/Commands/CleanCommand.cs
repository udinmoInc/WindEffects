using Serilog;
using IgniteBT.ModuleDiscovery;
using IgniteBT.BuildGraph;

namespace IgniteBT.Commands;

public static class CleanCommand
{
    public static async Task<int> Execute(string[] args)
    {
        var target = args.Length > 0 ? args[0] : "All";
        var config = GetArgValue(args, "--config", "Release");
        
        Log.Information("Clean Command");
        Log.Information("Target: {Target}", target);
        Log.Information("Configuration: {Config}", config);

        try
        {
            // Get engine directory
            var currentDir = Directory.GetCurrentDirectory();
            var engineDir = FindEngineRoot(currentDir);
            
            if (string.IsNullOrEmpty(engineDir) || !Directory.Exists(engineDir))
            {
                Log.Error("Could not find engine root directory from {CurrentDir}", currentDir);
                return 1;
            }
            
            Log.Information("Engine root: {EngineDir}", engineDir);
            
            // Discover modules
            var discovery = new ModuleDiscoverer(engineDir);
            var modules = await discovery.DiscoverModulesAsync();
            
            if (modules.Count == 0)
            {
                Log.Warning("No modules found to clean");
                return 0;
            }
            
            // Build dependency graph
            var graph = new DependencyGraph();
            graph.BuildFromModules(modules);
            
            // Get build order (reverse for cleaning)
            var buildOrder = graph.GetBuildOrder();
            buildOrder.Reverse();
            
            // Clean each module
            foreach (var node in buildOrder)
            {
                CleanModule(node, engineDir);
            }
            
            // Clean cache
            CleanCache(engineDir);
            
            Log.Information("Clean completed successfully");
            return 0;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Clean failed with exception");
            return 1;
        }
    }

    private static void CleanModule(BuildNode node, string engineDir)
    {
        Log.Information("Cleaning module: {ModuleName}", node.Name);
        
        // Clean intermediate directory for the module
        var intermediateDir = Path.Combine(engineDir, "Intermediate", node.Name);
        if (Directory.Exists(intermediateDir))
        {
            Directory.Delete(intermediateDir, true);
            Log.Information("Deleted intermediate directory: {Dir}", intermediateDir);
        }
        
        // Clean output directory for the module
        var outputDir = Path.Combine(engineDir, "Binaries", node.Name);
        if (Directory.Exists(outputDir))
        {
            Directory.Delete(outputDir, true);
            Log.Information("Deleted output directory: {Dir}", outputDir);
        }
        
        Log.Information("Module {ModuleName} cleaned successfully", node.Name);
    }

    private static void CleanCache(string engineDir)
    {
        Log.Information("Cleaning build cache");
        
        var cacheDir = Path.Combine(engineDir, "Intermediate", "IgniteBT", "Cache");
        if (Directory.Exists(cacheDir))
        {
            Directory.Delete(cacheDir, true);
            Log.Information("Deleted cache directory: {Dir}", cacheDir);
        }
        
        var databaseDir = Path.Combine(engineDir, "Intermediate", "IgniteBT", "Database");
        if (Directory.Exists(databaseDir))
        {
            Directory.Delete(databaseDir, true);
            Log.Information("Deleted database directory: {Dir}", databaseDir);
        }
    }

    static string GetArgValue(string[] args, string name, string defaultValue)
    {
        for (int i = 0; i < args.Length; i++)
        {
            if (args[i] == name && i + 1 < args.Length)
                return args[i + 1];
        }
        return defaultValue;
    }

    static string? FindEngineRoot(string startDir)
    {
        var dir = new DirectoryInfo(startDir);
        
        while (dir != null)
        {
            if (dir.Name.Equals("Engine", StringComparison.OrdinalIgnoreCase))
            {
                var sourceDir = Path.Combine(dir.FullName, "Source");
                if (Directory.Exists(sourceDir))
                {
                    return dir.FullName;
                }
            }
            
            var engineParent = Path.Combine(dir.FullName, "Engine", "Source");
            if (Directory.Exists(engineParent))
            {
                return Path.GetDirectoryName(engineParent);
            }
            
            dir = dir.Parent;
        }
        
        return null;
    }
}
