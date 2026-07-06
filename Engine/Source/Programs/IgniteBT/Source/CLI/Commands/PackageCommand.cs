using Serilog;

namespace IgniteBT.CLI;

public static class PackageCommand
{
    public static async Task<int> Execute(string[] args)
    {
        Log.Information("Package Command");
        Log.Error("Packaging is not implemented.");
        await Task.CompletedTask;
        return 1;
    }
}
