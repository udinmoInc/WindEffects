using Serilog;
using IgniteBT.Workspace.Modules;
using IgniteBT.Build.Layout;

namespace IgniteBT.CLI;

public static class ModulesCommand
{
    public static async Task<int> Execute(string[] args)
    {
        Log.Information("Modules Command - Listing all discovered modules");
        
        try
        {
            // Get engine directory - find the Engine root by looking for Source directory
            var currentDir = Directory.GetCurrentDirectory();
            var engineDir = BuildLayout.FindEngineRoot(currentDir);
            
            if (string.IsNullOrEmpty(engineDir) || !Directory.Exists(engineDir))
            {
                Log.Error("Could not find engine root directory from {CurrentDir}", currentDir);
                return 1;
            }
            
            Log.Information("Engine root: {EngineDir}", engineDir);
            
            var discovery = new ModuleDiscoverer(engineDir);
            var modules = await discovery.DiscoverModulesAsync();
            
            Console.WriteLine();
            Console.WriteLine($"Discovered {modules.Count} modules:");
            Console.WriteLine();
            
            foreach (var module in modules)
            {
                Console.WriteLine($"  {module.Name}");
                Console.WriteLine($"    Type: {module.Type}");
                Console.WriteLine($"    Path: {module.ModuleDirectory}");
                Console.WriteLine($"    Build.cs: {module.BuildCsPath}");
                Console.WriteLine();
            }
            
            return 0;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Failed to discover modules");
            return 1;
        }
    }
}
