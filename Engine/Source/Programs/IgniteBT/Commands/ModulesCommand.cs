using Serilog;

namespace IgniteBT.Commands;

public class ModulesCommand
{
    public async Task ExecuteAsync()
    {
        Log.Information("Modules Command - Listing all discovered modules");
        
        // TODO: Implement actual module discovery logic
        Log.Warning("Modules command not yet implemented - this is a placeholder");
        await Task.CompletedTask;
    }
}
