using Serilog;
using IgniteBT.Workspace.Modules;
using IgniteBT.Build.Graph;
using IgniteBT.Build.Layout;

namespace IgniteBT.CLI;

public static class GraphCommand
{
    public static async Task<int> Execute(string[] args)
    {
        Log.Information("Graph Command - Displaying build dependency graph");
        
        try
        {
            // Get engine directory
            var currentDir = Directory.GetCurrentDirectory();
            var engineDir = BuildLayout.FindEngineRoot(currentDir);
            
            if (string.IsNullOrEmpty(engineDir) || !Directory.Exists(engineDir))
            {
                Log.Error("Could not find engine root directory from {CurrentDir}", currentDir);
                return 1;
            }
            
            // Discover modules
            var discovery = new ModuleDiscoverer(engineDir);
            var modules = await discovery.DiscoverModulesAsync();
            
            if (modules.Count == 0)
            {
                Log.Warning("No modules found to display graph");
                return 0;
            }
            
            // Build graph
            var graph = new DependencyGraph();
            graph.BuildFromModules(modules);
            
            // Validate graph
            if (!graph.Validate())
            {
                Log.Error("Build graph validation failed");
                return 1;
            }
            
            // Get build order
            var buildOrder = graph.GetBuildOrder();
            
            Console.WriteLine();
            Console.WriteLine("Build Dependency Graph:");
            Console.WriteLine();
            
            // Display nodes and their dependencies
            foreach (var node in graph.GetNodes())
            {
                Console.WriteLine($"  {node.Name}");
                if (node.Dependencies.Count > 0)
                {
                    Console.WriteLine($"    Depends on: {string.Join(", ", node.Dependencies.Select(d => d.Name))}");
                }
                if (node.Dependents.Count > 0)
                {
                    Console.WriteLine($"    Required by: {string.Join(", ", node.Dependents.Select(d => d.Name))}");
                }
                Console.WriteLine();
            }
            
            Console.WriteLine("Build Order:");
            for (int i = 0; i < buildOrder.Count; i++)
            {
                Console.WriteLine($"  {i + 1}. {buildOrder[i].Name}");
            }
            
            return 0;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Failed to display build graph");
            return 1;
        }
    }
}
