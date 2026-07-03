using Serilog;
using IgniteBT.ModuleDiscovery;
using IgniteBT.BuildGraph;
using IgniteBT.BuildSystem;
using IgniteBT.Compiler;

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
            var currentDir = Directory.GetCurrentDirectory();
            var projectRoot = BuildLayout.FindProjectRoot(currentDir);
            var engineDir = BuildLayout.FindEngineRoot(currentDir);
            
            if (string.IsNullOrEmpty(projectRoot) || string.IsNullOrEmpty(engineDir) || !Directory.Exists(engineDir))
            {
                Log.Error("Could not find project root directory from {CurrentDir}", currentDir);
                return 1;
            }
            
            Log.Information("Project root: {ProjectRoot}", projectRoot);
            Log.Information("Engine root: {EngineDir}", engineDir);

            var buildConfig = ParseConfiguration(config);
            var layout = BuildLayout.Resolve(currentDir, GetCurrentPlatform(), buildConfig);
            
            // Discover modules
            var discovery = new ModuleDiscoverer(engineDir);
            var modules = await discovery.DiscoverModulesAsync();
            
            if (modules.Count == 0)
            {
                Log.Warning("No modules found to clean");
            }
            else
            {
                var graph = new DependencyGraph();
                graph.BuildFromModules(modules);
                var buildOrder = graph.GetBuildOrder();
                buildOrder.Reverse();

                foreach (var node in buildOrder)
                {
                    CleanModule(node, layout);
                }
            }

            CleanDirectory(layout.BuildRoot, "build directory");

            foreach (var legacyDir in BuildLayout.GetLegacyArtifactDirectories(projectRoot, engineDir))
            {
                CleanDirectory(legacyDir, "legacy artifact directory");
            }
            
            Log.Information("Clean completed successfully");
            return 0;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Clean failed with exception");
            return 1;
        }
    }

    private static void CleanModule(BuildNode node, BuildLayout layout)
    {
        Log.Information("Cleaning module: {ModuleName}", node.Name);

        CleanDirectory(layout.GetModuleObjectsDirectory(node.Name), $"objects for {node.Name}");
        CleanDirectory(layout.GetModuleGeneratedDirectory(node.Name), $"generated files for {node.Name}");
    }

    private static void CleanDirectory(string path, string description)
    {
        if (!Directory.Exists(path))
        {
            return;
        }

        Directory.Delete(path, true);
        Log.Information("Deleted {Description}: {Dir}", description, path);
    }

    private static BuildConfiguration ParseConfiguration(string config)
    {
        return config.ToLowerInvariant() switch
        {
            "debug" => BuildConfiguration.Debug,
            "development" or "dev" => BuildConfiguration.Development,
            "profile" => BuildConfiguration.Profile,
            "shipping" or "release" => BuildConfiguration.Shipping,
            _ => BuildConfiguration.Shipping
        };
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

    static string GetCurrentPlatform()
    {
        if (OperatingSystem.IsWindows())
            return "Windows";
        if (OperatingSystem.IsLinux())
            return "Linux";
        if (OperatingSystem.IsMacOS())
            return "Mac";
        return "Unknown";
    }
}
