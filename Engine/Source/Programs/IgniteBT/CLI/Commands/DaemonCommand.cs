using Serilog;
using IgniteBT.Daemon;

namespace IgniteBT.CLI;

public static class DaemonCommand
{
    public static int Execute(string[] args)
    {
        if (args.Length > 0 && args[0].Equals("serve", StringComparison.OrdinalIgnoreCase))
            return RunServer();

        var projectRoot = Directory.GetCurrentDirectory();
        using var daemon = new IgniteBTDaemon(projectRoot);
        daemon.Start();

        Console.WriteLine("IgniteBT daemon running. Press Enter to stop.");
        Console.ReadLine();
        return 0;
    }

    private static int RunServer()
    {
        var projectRoot = Directory.GetCurrentDirectory();
        var logDirectory = Path.Combine(projectRoot, "Build", "Logs");
        Directory.CreateDirectory(logDirectory);

        Log.Logger = new LoggerConfiguration()
            .MinimumLevel.Information()
            .WriteTo.Console()
            .WriteTo.File(Path.Combine(logDirectory, "IgniteBT-daemon-.log"), rollingInterval: RollingInterval.Day)
            .CreateLogger();

        using var server = new DaemonServer(projectRoot);
        server.Start();
        Log.Information("IgniteBT daemon server ready (PID {Pid})", Environment.ProcessId);

        var exit = new ManualResetEventSlim(false);
        Console.CancelKeyPress += (_, e) => { e.Cancel = true; exit.Set(); };
        exit.Wait();
        return 0;
    }
}
