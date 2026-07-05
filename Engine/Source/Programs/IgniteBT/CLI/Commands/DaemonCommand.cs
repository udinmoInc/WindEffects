using Serilog;
using IgniteBT.Daemon;

namespace IgniteBT.CLI;

public static class DaemonCommand
{
    public static int Execute(string[] args)
    {
        var projectRoot = Directory.GetCurrentDirectory();
        using var daemon = new IgniteBTDaemon(projectRoot);
        daemon.Start();

        Console.WriteLine("IgniteBT daemon running. Press Enter to stop.");
        Console.ReadLine();
        return 0;
    }
}
