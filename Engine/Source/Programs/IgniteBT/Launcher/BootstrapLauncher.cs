using System.Diagnostics;

namespace IgniteBT.Launcher;

/// <summary>
/// Locates the engine installation and forwards arguments to the resolved bootstrap tool.
/// </summary>
public static class BootstrapLauncher
{
    public static int Forward(string[] args, string? startDirectory = null)
    {
        var descriptor = ResolveDescriptor(startDirectory);
        BootstrapManifest.Refresh(descriptor);

        if (BootstrapManifest.TryLoad(descriptor.BootstrapManifestPath, out var manifest))
        {
            var requestedTool = args.Length > 0 && manifest.Tools.ContainsKey(args[0]) ? args[0] : null;
            var tool = BootstrapManifest.ResolveTool(manifest, requestedTool);
            if (tool != null && BootstrapManifest.IsToolRunnable(tool))
            {
                if (string.Equals(tool.Kind, "dotnet", StringComparison.OrdinalIgnoreCase)
                    && (string.IsNullOrWhiteSpace(tool.ExecutablePath) || !BootstrapManifest.IsDotNetExecutableUsable(tool.ExecutablePath)))
                {
                    return RunDotNetProject(tool.SourcePath, descriptor.EngineRoot, args);
                }

                if (!string.IsNullOrWhiteSpace(tool.ExecutablePath))
                {
                    return RunNativeExecutable(tool.ExecutablePath, descriptor.EngineRoot, args);
                }

                return RunDotNetProject(tool.SourcePath, descriptor.EngineRoot, args);
            }
        }

        return RunBootstrapEntry(descriptor, args);
    }

    public static EngineDescriptorData ResolveDescriptor(string? startDirectory = null)
    {
        var start = startDirectory ?? Directory.GetCurrentDirectory();
        if (!EngineDescriptor.TryFindEngineRoot(start, out var engineRoot)
            || !EngineDescriptor.TryLoad(engineRoot, out var descriptor))
        {
            throw new InvalidOperationException(
                $"Could not locate a WindEffects engine installation. Search upward for {EngineDescriptor.FileName} or set WE_PROJECT_ROOT.");
        }

        return descriptor;
    }

    private static int RunBootstrapEntry(EngineDescriptorData descriptor, string[] args)
    {
        if (string.IsNullOrWhiteSpace(descriptor.BootstrapEntryProjectPath)
            || !File.Exists(descriptor.BootstrapEntryProjectPath))
        {
            throw new FileNotFoundException(
                "Bootstrap entry project was not found.",
                descriptor.BootstrapEntryProjectPath);
        }

        return RunDotNetProject(descriptor.BootstrapEntryProjectPath, descriptor.EngineRoot, args);
    }

    private static int RunDotNetProject(string projectPath, string workingDirectory, string[] args)
    {
        var dotnet = FindOnPath(OperatingSystem.IsWindows() ? "dotnet.exe" : "dotnet")
            ?? throw new InvalidOperationException("dotnet SDK was not found on PATH.");

        var dotnetArgs = new List<string> { "run", "--project", projectPath, "--" };
        dotnetArgs.AddRange(args);
        return RunProcess(dotnet, dotnetArgs.ToArray(), workingDirectory);
    }

    private static int RunNativeExecutable(string executablePath, string workingDirectory, string[] args) =>
        RunProcess(executablePath, args, workingDirectory);

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

    private static string? FindOnPath(string fileName)
    {
        var pathEntries = (Environment.GetEnvironmentVariable("PATH") ?? string.Empty)
            .Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries);

        foreach (var entry in pathEntries)
        {
            var candidate = Path.Combine(entry.Trim(), fileName);
            if (File.Exists(candidate))
            {
                return candidate;
            }
        }

        return null;
    }
}
