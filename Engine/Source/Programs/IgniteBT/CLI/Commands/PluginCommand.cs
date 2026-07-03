using Serilog;

namespace IgniteBT.CLI;

public static class PluginCommand
{
    public static async Task<int> Execute(string[] args)
    {
        if (args.Length == 0)
        {
            PrintUsage();
            return 0;
        }

        var subcommand = args[0].ToLowerInvariant();
        var remaining = args.Skip(1).ToArray();

        return subcommand switch
        {
            "list" => await ListPlugins(),
            "build" => await BuildPlugin(remaining),
            "enable" => await SetPluginState(remaining, enabled: true),
            "disable" => await SetPluginState(remaining, enabled: false),
            _ => PrintUsage()
        };
    }

    private static Task<int> ListPlugins()
    {
        Log.Information("Plugin list is not yet implemented.");
        return Task.FromResult(0);
    }

    private static Task<int> BuildPlugin(string[] args)
    {
        if (args.Length == 0)
        {
            Log.Error("Usage: we plugin build <name>");
            return Task.FromResult(1);
        }

        Log.Information("Building plugin: {Name}", args[0]);
        Log.Warning("Plugin build is not yet implemented.");
        return Task.FromResult(0);
    }

    private static Task<int> SetPluginState(string[] args, bool enabled)
    {
        if (args.Length == 0)
        {
            Log.Error("Usage: we plugin {Action} <name>", enabled ? "enable" : "disable");
            return Task.FromResult(1);
        }

        Log.Information("{Action} plugin: {Name}", enabled ? "Enabling" : "Disabling", args[0]);
        Log.Warning("Plugin management is not yet implemented.");
        return Task.FromResult(0);
    }

    private static int PrintUsage()
    {
        Console.WriteLine("Plugin commands:");
        Console.WriteLine("  we plugin list");
        Console.WriteLine("  we plugin build <name>");
        Console.WriteLine("  we plugin enable <name>");
        Console.WriteLine("  we plugin disable <name>");
        return 0;
    }
}
