using Serilog;
using IgniteBT.Workspace.SDK;

namespace IgniteBT.CLI;

public static class SdkCommand
{
    public static async Task<int> Execute(string[] args)
    {
        var subcommand = args.Length > 0 ? args[0].ToLowerInvariant() : "list";
        var remaining = args.Skip(1).ToArray();

        SDKManager.Instance.Initialize();

        return subcommand switch
        {
            "list" => ListSdks(),
            "detect" => await DetectSdks(),
            "validate" => await ValidateSdk(remaining),
            _ => PrintUsage()
        };
    }

    private static int ListSdks()
    {
        var sdks = SDKManager.Instance.GetAllSDKs();
        if (sdks.Count == 0)
        {
            Log.Warning("No SDKs discovered. Run `we sdk detect`.");
            return 0;
        }

        Console.WriteLine($"{"Name",-20} {"Version",-16} {"Status",-10} Path");
        Console.WriteLine(new string('-', 100));
        foreach (var sdk in sdks.Values.OrderBy(s => s.Name, StringComparer.OrdinalIgnoreCase))
        {
            Console.WriteLine($"{sdk.Name,-20} {sdk.Version,-16} {(sdk.IsValid ? "Valid" : "Invalid"),-10} {sdk.RootPath}");
        }

        return 0;
    }

    private static async Task<int> DetectSdks()
    {
        Log.Information("Detecting SDKs...");
        var results = await SDKManager.Instance.DetectAllAsync();
        Log.Information("Detected {Count} SDK(s)", results.Count);
        return ListSdks();
    }

    private static async Task<int> ValidateSdk(string[] args)
    {
        if (args.Length == 0)
        {
            Log.Error("Usage: we sdk validate <name>");
            return 1;
        }

        var result = await SDKManager.Instance.DetectSDKAsync(args[0]);
        if (!result.Success || result.Value == null)
        {
            Log.Error("SDK not found: {Name}", args[0]);
            return 1;
        }

        if (result.Value.IsValid)
        {
            Log.Information("SDK {Name} is valid at {Path}", result.Value.Name, result.Value.RootPath);
            return 0;
        }

        Log.Error("SDK {Name} validation failed", result.Value.Name);
        foreach (var error in result.Value.ValidationErrors)
        {
            Log.Error("  {Error}", error);
        }

        return 1;
    }

    private static int PrintUsage()
    {
        Console.WriteLine("SDK commands:");
        Console.WriteLine("  we sdk list");
        Console.WriteLine("  we sdk detect");
        Console.WriteLine("  we sdk validate <name>");
        return 0;
    }
}
