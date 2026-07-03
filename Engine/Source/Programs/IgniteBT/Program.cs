using System.CommandLine;
using Serilog;
using IgniteBT.Commands;
using IgniteBT.BuildSystem;

namespace IgniteBT;

class Program
{
    static async Task<int> Main(string[] args)
    {
        var projectRoot = BuildLayout.FindProjectRoot(Directory.GetCurrentDirectory()) ?? Directory.GetCurrentDirectory();
        var logDirectory = Path.Combine(projectRoot, "Build", "Logs");
        Directory.CreateDirectory(logDirectory);

        // Initialize logging
        Log.Logger = new LoggerConfiguration()
            .MinimumLevel.Information()
            .WriteTo.Console()
            .WriteTo.File(Path.Combine(logDirectory, "IgniteBT-.log"), rollingInterval: RollingInterval.Day)
            .CreateLogger();

        Log.Information("IgniteBT - WindEffects Build Tool v1.0.0");
        Log.Information("========================================");

        try
        {
            // Simple command parsing
            if (args.Length == 0)
            {
                PrintUsage();
                return 0;
            }

            var command = args[0].ToLower();
            var remainingArgs = args.Skip(1).ToArray();

            return command switch
            {
                "build" => await BuildCommand.Execute(remainingArgs),
                "clean" => await CleanCommand.Execute(remainingArgs),
                "rebuild" => await RebuildCommand.Execute(remainingArgs),
                "graph" => await GraphCommand.Execute(remainingArgs),
                "modules" => await ModulesCommand.Execute(remainingArgs),
                "doctor" => await DoctorCommand.Execute(remainingArgs),
                "version" => VersionCommand.Execute(),
                _ => PrintUsage()
            };
        }
        catch (Exception ex)
        {
            Log.Fatal(ex, "Fatal error occurred");
            return 1;
        }
        finally
        {
            Log.CloseAndFlush();
        }
    }

    static int PrintUsage()
    {
        Console.WriteLine("IgniteBT - WindEffects Build Tool v1.0.0");
        Console.WriteLine();
        Console.WriteLine("Usage:");
        Console.WriteLine("  ignitebt build [target] [--config Debug|Release] [--platform Windows|Linux|Mac] [--clean] [--jobs N]");
        Console.WriteLine("  ignitebt clean [target]");
        Console.WriteLine("  ignitebt rebuild [target]");
        Console.WriteLine("  ignitebt graph");
        Console.WriteLine("  ignitebt modules");
        Console.WriteLine("  ignitebt doctor");
        Console.WriteLine("  ignitebt version");
        return 0;
    }

    static string GetCurrentPlatform()
    {
        if (OperatingSystem.IsWindows())
            return "Windows";
        if (OperatingSystem.IsLinux())
            return "Linux";
        if (OperatingSystem.IsMacOS())
            return "Mac";
        return "Unknown";
    }
}
