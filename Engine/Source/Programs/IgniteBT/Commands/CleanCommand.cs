using Serilog;

namespace IgniteBT.Commands;

public class CleanCommand
{
    public async Task ExecuteAsync(string? target)
    {
        Log.Information("Clean Command");
        Log.Information("Target: {Target}", target ?? "All");
        
        // TODO: Implement actual clean logic
        Log.Warning("Clean command not yet implemented - this is a placeholder");
        await Task.CompletedTask;
    }
}
