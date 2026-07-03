using Serilog;
using IgniteBT.CLI;
using IgniteBT.Build.Layout;
using IgniteBT.Core.Launcher;

namespace IgniteBT;

class Program
{
    static async Task<int> Main(string[] args)
    {
        var descriptor = BootstrapLauncher.ResolveDescriptor();
        BootstrapManifest.Refresh(descriptor);

        var logDirectory = Path.Combine(descriptor.BuildRoot, "Logs");
        Directory.CreateDirectory(logDirectory);

        Log.Logger = new LoggerConfiguration()
            .MinimumLevel.Information()
            .WriteTo.Console()
            .WriteTo.File(Path.Combine(logDirectory, "IgniteBT-.log"), rollingInterval: RollingInterval.Day)
            .CreateLogger();

        Log.Information("IgniteBT - WindEffects Build Tool v1.0.0");
        Log.Information("========================================");

        try
        {
            if (args.Length == 0)
            {
                return PrintUsage();
            }

            var command = args[0].ToLower();
            var remainingArgs = args.Skip(1).ToArray();

            return command switch
            {
                "build" => await BuildCommand.Execute(remainingArgs),
                "clean" => await CleanCommand.Execute(remainingArgs),
                "rebuild" => await RebuildCommand.Execute(remainingArgs),
                "package" => await PackageCommand.Execute(remainingArgs),
                "run" => await RunCommand.Execute(remainingArgs),
                "project" => await ProjectCommand.Execute(remainingArgs),
                "plugin" => await PluginCommand.Execute(remainingArgs),
                "sdk" => await SdkCommand.Execute(remainingArgs),
                "setup" => await SetupCommand.Execute(remainingArgs),
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
        Console.WriteLine("WindEffects CLI (we) / IgniteBT v1.0.0");
        Console.WriteLine();
        Console.WriteLine("Usage:");
        Console.WriteLine("  we build [target] [--config Debug|Development|Shipping] [--platform Windows|Linux|Mac] [--jobs N]");
        Console.WriteLine("  we clean [target] [--config Debug|Development|Shipping]");
        Console.WriteLine("  we rebuild [target] [--config Debug|Development|Shipping]");
        Console.WriteLine("  we package [target]");
        Console.WriteLine("  we run [--target Editor] [--config Debug]");
        Console.WriteLine("  we project list|open|create");
        Console.WriteLine("  we plugin list|build|enable|disable");
        Console.WriteLine("  we sdk list|detect|validate");
        Console.WriteLine("  we setup");
        Console.WriteLine("  we doctor");
        Console.WriteLine("  we graph");
        Console.WriteLine("  we modules");
        Console.WriteLine("  we version");
        Console.WriteLine();
        Console.WriteLine("Install the `we` command globally with: we setup");
        return 0;
    }
}
