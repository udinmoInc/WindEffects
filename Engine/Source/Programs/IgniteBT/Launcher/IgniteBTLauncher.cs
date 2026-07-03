using System.Diagnostics;

namespace IgniteBT.Launcher;

/// <summary>
/// Spawns IgniteBT with forwarded CLI arguments.
/// </summary>
public static class IgniteBTLauncher
{
    public static int Forward(string[] args, string? startDirectory = null)
    {
        var location = EngineInstallation.Resolve(startDirectory);
        var igniteBtExecutable = EngineInstallation.ResolveIgniteBtExecutable(location);

        if (!string.IsNullOrWhiteSpace(igniteBtExecutable))
        {
            return RunProcess(igniteBtExecutable, args, location.ProjectRoot);
        }

        if (!File.Exists(location.IgniteBtProjectPath))
        {
            throw new FileNotFoundException(
                "IgniteBT is not built and the IgniteBT project file could not be found.",
                location.IgniteBtProjectPath);
        }

        var dotnet = FindDotNetExecutable()
            ?? throw new InvalidOperationException("dotnet SDK was not found on PATH. Install .NET 8 SDK or build IgniteBT first.");

        return RunProcess(dotnet, BuildDotNetArgs(location.IgniteBtProjectPath, args), location.ProjectRoot);
    }

    private static string[] BuildDotNetArgs(string csprojPath, string[] args)
    {
        var dotnetArgs = new List<string> { "run", "--project", csprojPath, "--" };
        dotnetArgs.AddRange(args);
        return dotnetArgs.ToArray();
    }

    private static int RunProcess(string executable, string[] args, string workingDirectory)
    {
        var startInfo = new ProcessStartInfo
        {
            FileName = executable,
            WorkingDirectory = workingDirectory,
            UseShellExecute = false,
        };

        foreach (var arg in args)
        {
            startInfo.ArgumentList.Add(arg);
        }

        using var process = Process.Start(startInfo)
            ?? throw new InvalidOperationException($"Failed to start process: {executable}");

        process.WaitForExit();
        return process.ExitCode;
    }

    private static string? FindDotNetExecutable()
    {
        var pathEntries = (Environment.GetEnvironmentVariable("PATH") ?? string.Empty)
            .Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries);

        foreach (var entry in pathEntries)
        {
            var candidate = Path.Combine(entry.Trim(), OperatingSystem.IsWindows() ? "dotnet.exe" : "dotnet");
            if (File.Exists(candidate))
            {
                return candidate;
            }
        }

        return null;
    }
}
