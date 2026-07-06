using Serilog;
using Serilog.Sinks.File;
using IgniteBT.CLI;
using IgniteBT.Daemon;

namespace IgniteBT;

class Program
{
    static async Task<int> Main(string[] args)
    {
        IgniteBT.Core.Launcher.BuildEnvironment.Configure();

        if (args.Length > 0 && args[0].Equals("daemon", StringComparison.OrdinalIgnoreCase)
            && args.Length > 1 && args[1].Equals("serve", StringComparison.OrdinalIgnoreCase))
        {
            return DaemonCommand.Execute(args.Skip(1).ToArray());
        }

        if (args.Length > 0 && args[0].Equals("build", StringComparison.OrdinalIgnoreCase))
        {
            var buildArgs = args.Skip(1).ToArray();
            var daemonResult = DaemonHost.TryForwardBuild(buildArgs);
            if (daemonResult.HasValue)
                return daemonResult.Value;

            var fastNoOp = BuildCommand.TryFastNoOp(buildArgs);
            if (fastNoOp.HasValue)
                return fastNoOp.Value;
        }

        return await RunWithLoggingAsync(args);
    }

    static async Task<int> RunWithLoggingAsync(string[] args)
    {
        Serilog.Log.Logger = CreateLogger();

        Serilog.Log.Information("IgniteBT - WindEffects Build Tool v1.0.0");
        Serilog.Log.Information("========================================");

        try
        {
            if (args.Length == 0)
                return PrintUsage();

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
                "daemon" => DaemonCommand.Execute(remainingArgs),
                "benchmark" => await BenchmarkCommand.Execute(remainingArgs),
                _ => PrintUsage()
            };
        }
        catch (Exception ex)
        {
            Serilog.Log.Fatal(ex, "Fatal error occurred");
            return 1;
        }
        finally
        {
            Serilog.Log.CloseAndFlush();
        }
    }

    static Serilog.ILogger CreateLogger()
    {
        try
        {
            var descriptor = IgniteBT.Core.Launcher.BootstrapLauncher.ResolveDescriptor();
            IgniteBT.Core.Launcher.BootstrapManifest.Refresh(descriptor);
            var logDirectory = Path.Combine(descriptor.BuildRoot, "Logs");
            Directory.CreateDirectory(logDirectory);
            return new Serilog.LoggerConfiguration()
                .MinimumLevel.Information()
                .WriteTo.Console()
                .WriteTo.File(Path.Combine(logDirectory, "IgniteBT-.log"), rollingInterval: RollingInterval.Day)
                .CreateLogger();
        }
        catch
        {
            return new Serilog.LoggerConfiguration()
                .MinimumLevel.Information()
                .WriteTo.Console()
                .CreateLogger();
        }
    }

    static int PrintUsage()
    {
        Console.WriteLine("WindEffects CLI (we) / IgniteBT v1.0.0");
        Console.WriteLine();
        Console.WriteLine("Usage:");
        Console.WriteLine("  we build [target] [--target NAME] [--config Debug|Development|Shipping] [--platform Win64|Windows|Linux|Mac] [--jobs N] [--clean]");
        Console.WriteLine("  we clean [target] [--target NAME] [--config Debug|Development|Shipping] [--platform Win64|Windows|Linux|Mac]");
        Console.WriteLine("  we rebuild [target] [--target NAME] [--config Debug|Development|Shipping] [--platform Win64|Windows|Linux|Mac] [--jobs N]");
        Console.WriteLine("  we run [--target Editor] [--config Debug]");
        Console.WriteLine("  we sdk list|detect|validate");
        Console.WriteLine("  we setup");
        Console.WriteLine("  we doctor");
        Console.WriteLine("  we daemon [serve]");
        Console.WriteLine("  we benchmark [--jobs 8,16,32]");
        Console.WriteLine("  we graph");
        Console.WriteLine("  we modules");
        Console.WriteLine("  we version");
        Console.WriteLine();
        Console.WriteLine("Install the `we` command globally with: we setup");
        return 0;
    }
}
