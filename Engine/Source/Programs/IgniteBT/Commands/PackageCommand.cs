using Serilog;

namespace IgniteBT.Commands;

public static class PackageCommand
{
    public static async Task<int> Execute(string[] args)
    {
        Log.Information("Package Command");
        Log.Warning("Packaging is not yet implemented. This command will produce distributable editor/runtime packages.");
        await Task.CompletedTask;
        return 0;
    }
}
