using Serilog;
using IgniteBT.Workspace.Modules;
using IgniteBT.Build.Graph;
using IgniteBT.Build.Layout;
using IgniteBT.Build.Compiler;

namespace IgniteBT.CLI;

public static class CleanCommand
{
    public static async Task<int> Execute(string[] args)
    {
        var parsed = CommandSchemas.Clean.Parse(args);
        if (!CommandLineHelpers.TryReportErrors(parsed))
        {
            return 1;
        }

        var target = parsed.ResolveTarget();
        var config = parsed.GetOption("config", "Debug");
        var platform = CommandLineHelpers.NormalizePlatform(parsed.GetOption("platform", string.Empty));
        
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

            var buildConfig = CommandLineHelpers.ParseConfiguration(config);
            var layout = BuildLayout.Resolve(currentDir, platform, buildConfig);
            var outputLayout = new OutputLayout(layout, engineDir);
            
            var discovery = new ModuleDiscoverer(engineDir, config, platform);
            var modules = await discovery.DiscoverModulesAsync();
            
            if (modules.Count == 0)
            {
                Log.Warning("No modules found to clean");
            }
            else
            {
                outputLayout.RegisterModules(modules);
                var graph = new DependencyGraph();
                graph.BuildFromModules(modules);
                var buildOrder = graph.GetBuildOrder();
                buildOrder.Reverse();

                foreach (var node in buildOrder)
                {
                    CleanModule(node, layout, outputLayout);
                }
            }

            CleanBuildTree(layout);

            foreach (var legacyDir in BuildLayout.GetLegacyArtifactDirectories(projectRoot, engineDir))
            {
                CleanDirectory(legacyDir, "legacy artifact directory");
            }
            
            Log.Information("Clean completed successfully");
            return 0;
        }
        catch (ModuleDiscoveryException ex)
        {
            Log.Error(ex, "Module discovery failed");
            return 1;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Clean failed with exception");
            return 1;
        }
    }

    private static void CleanBuildTree(BuildLayout layout)
    {
        if (!Directory.Exists(layout.BuildRoot))
        {
            return;
        }

        foreach (var entry in Directory.EnumerateFileSystemEntries(layout.BuildRoot))
        {
            var name = Path.GetFileName(entry);
            if (string.Equals(name, "Intermediate", StringComparison.OrdinalIgnoreCase))
            {
                CleanIntermediateTree(entry, layout.IgniteBtOutputRoot);
                continue;
            }

            CleanDirectory(entry, $"build/{name}");
        }
    }

    private static void CleanIntermediateTree(string intermediateRoot, string igniteBtRoot)
    {
        if (!Directory.Exists(intermediateRoot))
        {
            return;
        }

        foreach (var entry in Directory.EnumerateFileSystemEntries(intermediateRoot))
        {
            if (IsSameOrChildPath(entry, igniteBtRoot))
            {
                Log.Information("Skipping IgniteBT tool output (in use by this session): {Dir}", entry);
                continue;
            }

            CleanDirectory(entry, $"intermediate/{Path.GetFileName(entry)}");
        }
    }

    private static bool IsSameOrChildPath(string path, string root)
    {
        var fullPath = Path.GetFullPath(path).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
        var fullRoot = Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
        return fullPath.Equals(fullRoot, StringComparison.OrdinalIgnoreCase)
            || fullPath.StartsWith(fullRoot + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase);
    }

    private static void CleanModule(BuildNode node, BuildLayout layout, OutputLayout outputLayout)
    {
        Log.Information("Cleaning module: {ModuleName}", node.Name);

        CleanDirectory(layout.GetModuleObjectsDirectory(node.Name), $"objects for {node.Name}");
        CleanDirectory(layout.GetModuleGeneratedDirectory(node.Name), $"generated files for {node.Name}");
        CleanDirectory(layout.GetModuleImportLibraryDirectory(node.Name), $"import library for {node.Name}");

        TryDeleteFile(outputLayout.GetModuleBinaryPath(node.Module), $"binary for {node.Name}");
    }

    private static void CleanDirectory(string path, string description)
    {
        if (!Directory.Exists(path))
        {
            if (File.Exists(path))
            {
                TryDeleteFile(path, description);
            }
            return;
        }

        try
        {
            Directory.Delete(path, true);
            Log.Information("Deleted {Description}: {Dir}", description, path);
        }
        catch (UnauthorizedAccessException ex)
        {
            Log.Warning(ex, "Could not fully delete {Description} (files may be in use): {Dir}", description, path);
            TryDeleteDirectoryContents(path, description);
        }
        catch (IOException ex)
        {
            Log.Warning(ex, "Could not fully delete {Description}: {Dir}", description, path);
            TryDeleteDirectoryContents(path, description);
        }
    }

    private static void TryDeleteDirectoryContents(string directory, string description)
    {
        foreach (var file in Directory.EnumerateFiles(directory, "*", SearchOption.AllDirectories))
        {
            TryDeleteFile(file, description);
        }

        foreach (var dir in Directory.EnumerateDirectories(directory, "*", SearchOption.AllDirectories).OrderByDescending(d => d.Length))
        {
            try
            {
                if (Directory.EnumerateFileSystemEntries(dir).Any())
                {
                    continue;
                }

                Directory.Delete(dir, false);
            }
            catch (Exception ex)
            {
                Log.Debug(ex, "Skipped directory: {Dir}", dir);
            }
        }
    }

    private static void TryDeleteFile(string path, string description)
    {
        try
        {
            File.Delete(path);
            Log.Debug("Deleted file from {Description}: {File}", description, path);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Skipped locked file in {Description}: {File}", description, path);
        }
    }
}
