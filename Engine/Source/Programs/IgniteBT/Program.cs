using System.CommandLine;
using Serilog;

namespace IgniteBT;

class Program
{
    static async Task<int> Main(string[] args)
    {
        // Initialize logging
        Log.Logger = new LoggerConfiguration()
            .MinimumLevel.Information()
            .WriteTo.Console()
            .WriteTo.File("Logs/IgniteBT-.log", rollingInterval: RollingInterval.Day)
            .CreateLogger();

        Log.Information("IgniteBT - WindEffects Build Tool v1.0.0");
        Log.Information("========================================");

        try
        {
            // Create root command
            var rootCommand = new RootCommand("IgniteBT - WindEffects Build Tool")
            {
                new Command("build", "Build specified targets")
                {
                    new Argument<string>("target", "Target to build (e.g., Editor, Runtime, Launcher)")
                    {
                        Arity = ArgumentArity.ZeroOrOne
                    },
                    new Option<string>("--config", () => "Release", "Build configuration (Debug, Release)"),
                    new Option<string>("--platform", () => GetCurrentPlatform(), "Target platform"),
                    new Option<bool>("--clean", () => false, "Clean before building"),
                    new Option<int>("--jobs", () => Environment.ProcessorCount, "Number of parallel jobs")
                },
                new Command("clean", "Clean build artifacts")
                {
                    new Argument<string>("target", "Target to clean")
                    {
                        Arity = ArgumentArity.ZeroOrOne
                    }
                },
                new Command("rebuild", "Rebuild specified targets")
                {
                    new Argument<string>("target", "Target to rebuild")
                    {
                        Arity = ArgumentArity.ZeroOrOne
                    }
                },
                new Command("graph", "Display build dependency graph"),
                new Command("modules", "List all discovered modules"),
                new Command("doctor", "Diagnose build system issues"),
                new Command("version", "Display version information")
            };

            // Set command handlers
            rootCommand.Subcommands["build"].SetHandler(async (string? target, string config, string platform, bool clean, int jobs) =>
            {
                var buildCommand = new Commands.BuildCommand();
                await buildCommand.ExecuteAsync(target, config, platform, clean, jobs);
            },
            rootCommand.Subcommands["build"].Arguments[0],
            rootCommand.Subcommands["build"].Options[0],
            rootCommand.Subcommands["build"].Options[1],
            rootCommand.Subcommands["build"].Options[2],
            rootCommand.Subcommands["build"].Options[3]);

            rootCommand.Subcommands["clean"].SetHandler(async (string? target) =>
            {
                var cleanCommand = new Commands.CleanCommand();
                await cleanCommand.ExecuteAsync(target);
            },
            rootCommand.Subcommands["clean"].Arguments[0]);

            rootCommand.Subcommands["rebuild"].SetHandler(async (string? target) =>
            {
                var rebuildCommand = new Commands.RebuildCommand();
                await rebuildCommand.ExecuteAsync(target);
            },
            rootCommand.Subcommands["rebuild"].Arguments[0]);

            rootCommand.Subcommands["graph"].SetHandler(async () =>
            {
                var graphCommand = new Commands.GraphCommand();
                await graphCommand.ExecuteAsync();
            });

            rootCommand.Subcommands["modules"].SetHandler(async () =>
            {
                var modulesCommand = new Commands.ModulesCommand();
                await modulesCommand.ExecuteAsync();
            });

            rootCommand.Subcommands["doctor"].SetHandler(async () =>
            {
                var doctorCommand = new Commands.DoctorCommand();
                await doctorCommand.ExecuteAsync();
            });

            rootCommand.Subcommands["version"].SetHandler(() =>
            {
                Console.WriteLine("IgniteBT v1.0.0");
                Console.WriteLine("WindEffects Build Tool");
                Console.WriteLine(".NET 8.0");
            });

            return await rootCommand.InvokeAsync(args);
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
