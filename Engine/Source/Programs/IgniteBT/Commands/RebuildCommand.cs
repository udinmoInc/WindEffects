using Serilog;

namespace IgniteBT.Commands;

public class RebuildCommand
{
    public async Task ExecuteAsync(string? target)
    {
        Log.Information("Rebuild Command");
        Log.Information("Target: {Target}", target ?? "All");
        
        // TODO: Implement actual rebuild logic
        Log.Warning("Rebuild command not yet implemented - this is a placeholder");
        await Task.CompletedTask;
    }
}
