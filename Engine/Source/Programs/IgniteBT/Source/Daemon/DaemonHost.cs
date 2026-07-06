using System.Diagnostics;
using IgniteBT.Core.Launcher;

namespace IgniteBT.Daemon;

/// <summary>
/// Ensures a per-project daemon is running and routes commands over IPC.
/// </summary>
public static class DaemonHost
{
    public static int? TryForwardBuild(string[] args)
    {
        if (!string.IsNullOrEmpty(Environment.GetEnvironmentVariable("IGNITEBT_SKIP_DAEMON")))
            return null;

        var cwd = Directory.GetCurrentDirectory();
        var projectRoot = IgniteBT.Build.Layout.BuildLayout.FindProjectRoot(cwd);
        if (string.IsNullOrEmpty(projectRoot)) return null;

        if (!IsDaemonRunning(projectRoot))
            TryStartDaemon(projectRoot);

        if (!DaemonClient.TryExecute(projectRoot, "build", args, cwd, out var response, connectTimeoutMs: 150))
            return null;

        if (!string.IsNullOrEmpty(response!.Output))
            Console.WriteLine(response.Output);
        if (!string.IsNullOrEmpty(response.Error))
            Console.Error.WriteLine(response.Error);

        return response.ExitCode;
    }

    public static bool IsDaemonRunning(string projectRoot)
    {
        var pidPath = DaemonPaths.GetPidFilePath(projectRoot);
        if (!File.Exists(pidPath)) return false;
        if (!int.TryParse(File.ReadAllText(pidPath).Trim(), out var pid)) return false;

        try
        {
            var proc = Process.GetProcessById(pid);
            return !proc.HasExited;
        }
        catch
        {
            return false;
        }
    }

    public static bool TryStartDaemon(string projectRoot)
    {
        if (IsDaemonRunning(projectRoot)) return true;

        var exe = ResolveIgniteBtExecutable();
        if (string.IsNullOrEmpty(exe)) return false;

        BuildEnvironment.Configure(projectRoot);

        var startInfo = new ProcessStartInfo
        {
            FileName = exe,
            Arguments = "daemon serve",
            WorkingDirectory = projectRoot,
            UseShellExecute = false,
            CreateNoWindow = true,
        };
        BuildEnvironment.ApplyToProcessEnvironment(startInfo, projectRoot);

        try
        {
            Process.Start(startInfo);
            Thread.Sleep(200);
            return IsDaemonRunning(projectRoot);
        }
        catch
        {
            return false;
        }
    }

    private static string? ResolveIgniteBtExecutable()
    {
        var env = Environment.GetEnvironmentVariable("IGNITEBT_EXECUTABLE");
        if (!string.IsNullOrEmpty(env) && File.Exists(env)) return env;

        var baseDir = AppContext.BaseDirectory;
        var candidate = Path.Combine(baseDir, OperatingSystem.IsWindows() ? "IgniteBT.exe" : "IgniteBT");
        if (File.Exists(candidate)) return candidate;

        return null;
    }
}
