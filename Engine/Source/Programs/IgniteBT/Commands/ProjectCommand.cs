using Serilog;

namespace IgniteBT.Commands;

public static class ProjectCommand
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
            "list" => await ListProjects(),
            "open" => await OpenProject(remaining),
            "create" => await CreateProject(remaining),
            _ => PrintUsage()
        };
    }

    private static Task<int> ListProjects()
    {
        Log.Information("Project list is not yet implemented.");
        return Task.FromResult(0);
    }

    private static Task<int> OpenProject(string[] args)
    {
        if (args.Length == 0)
        {
            Log.Error("Usage: we project open <path>");
            return Task.FromResult(1);
        }

        Log.Information("Opening project: {Path}", args[0]);
        Log.Warning("Project open is not yet implemented.");
        return Task.FromResult(0);
    }

    private static Task<int> CreateProject(string[] args)
    {
        if (args.Length == 0)
        {
            Log.Error("Usage: we project create <name>");
            return Task.FromResult(1);
        }

        Log.Information("Creating project: {Name}", args[0]);
        Log.Warning("Project creation is not yet implemented.");
        return Task.FromResult(0);
    }

    private static int PrintUsage()
    {
        Console.WriteLine("Project commands:");
        Console.WriteLine("  we project list");
        Console.WriteLine("  we project open <path>");
        Console.WriteLine("  we project create <name>");
        return 0;
    }
}
