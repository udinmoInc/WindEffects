// ==============================================================================
// WindEffects — IgniteBT — PluginCommand
// Source file for the IgniteBT module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using Serilog;

namespace IgniteBT.CLI;

public static class PluginCommand
{
    public static async Task<int> Execute(string[] args)
    {
        if (args.Length == 0)
        {
            PrintUsage();
            return 1;
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
        Log.Error("Plugin list is not implemented.");
        return Task.FromResult(1);
    }

    private static Task<int> BuildPlugin(string[] args)
    {
        if (args.Length == 0)
        {
            Log.Error("Usage: we plugin build <name>");
            return Task.FromResult(1);
        }

        Log.Error("Plugin build is not implemented.");
        return Task.FromResult(1);
    }

    private static Task<int> SetPluginState(string[] args, bool enabled)
    {
        if (args.Length == 0)
        {
            Log.Error("Usage: we plugin {Action} <name>", enabled ? "enable" : "disable");
            return Task.FromResult(1);
        }

        Log.Error("Plugin management is not implemented.");
        return Task.FromResult(1);
    }

    private static int PrintUsage()
    {
        Console.WriteLine("Plugin commands:");
        Console.WriteLine("  we plugin list");
        Console.WriteLine("  we plugin build <name>");
        Console.WriteLine("  we plugin enable <name>");
        Console.WriteLine("  we plugin disable <name>");
        return 1;
    }
}
