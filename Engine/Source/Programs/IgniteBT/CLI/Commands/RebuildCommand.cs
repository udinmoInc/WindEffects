using Serilog;

namespace IgniteBT.CLI;

public static class RebuildCommand
{
    public static async Task<int> Execute(string[] args)
    {
        var target = args.Length > 0 ? args[0] : "All";
        
        Log.Information("Rebuild Command");
        Log.Information("Target: {Target}", target);
        Log.Information("Rebuild is equivalent to Clean followed by Build");
        
        // First clean
        Log.Information("Step 1: Cleaning...");
        var cleanResult = await CleanCommand.Execute(args);
        
        if (cleanResult != 0)
        {
            Log.Error("Clean failed, aborting rebuild");
            return cleanResult;
        }
        
        // Then build
        Log.Information("Step 2: Building...");
        var buildResult = await BuildCommand.Execute(args);
        
        if (buildResult != 0)
        {
            Log.Error("Build failed");
            return buildResult;
        }
        
        Log.Information("Rebuild completed successfully");
        return 0;
    }
}
