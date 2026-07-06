using Serilog;
using IgniteBT.Core.Launcher;

namespace IgniteBT.CLI;

public static class SetupCommand
{
    public static async Task<int> Execute(string[] args)
    {
        Log.Information("Setup Command - Installing WindEffects CLI launcher");

        try
        {
            var location = EngineInstallation.Resolve();
            EngineInstallation.WriteInstallManifest(location);

            var launcherDir = EngineInstallation.GetInstalledLauncherDirectory();
            Directory.CreateDirectory(launcherDir);

            if (OperatingSystem.IsWindows())
            {
                InstallWindowsLauncher(location, launcherDir);
            }
            else
            {
                InstallUnixLauncher(location, launcherDir);
            }

            Log.Information("Installed `we` launcher to {LauncherDir}", launcherDir);
            Log.Information("Project root: {ProjectRoot}", location.ProjectRoot);
            Log.Information("Restart your terminal, then run: we doctor");
            await Task.CompletedTask;
            return 0;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Setup failed");
            return 1;
        }
    }

    private static void InstallWindowsLauncher(EngineInstallation.EngineLocation location, string launcherDir)
    {
        var sourcePs1 = Path.Combine(location.LauncherRoot, "we.ps1");
        var sourceBat = Path.Combine(location.LauncherRoot, "we.bat");

        if (!File.Exists(sourcePs1))
        {
            throw new FileNotFoundException("Repository launcher was not found.", sourcePs1);
        }

        var installedPs1 = Path.Combine(launcherDir, "we.ps1");
        var installedBat = Path.Combine(launcherDir, "we.bat");

        File.Copy(sourcePs1, installedPs1, overwrite: true);

        var batContents = $"""
            @echo off
            setlocal
            set "WE_PROJECT_ROOT={location.ProjectRoot}"
            set "WE_ENGINE_ROOT={location.EngineRoot}"
            powershell -NoProfile -ExecutionPolicy Bypass -File "{installedPs1}" %*
            exit /b %ERRORLEVEL%
            """;

        File.WriteAllText(installedBat, batContents);

        AddDirectoryToUserPath(launcherDir);
        AddDirectoryToUserPath(location.LauncherRoot);
    }

    private static void InstallUnixLauncher(EngineInstallation.EngineLocation location, string launcherDir)
    {
        var sourceLauncher = Path.Combine(location.LauncherRoot, "we");
        if (!File.Exists(sourceLauncher))
        {
            throw new FileNotFoundException("Repository launcher was not found.", sourceLauncher);
        }

        var installedLauncher = Path.Combine(launcherDir, "we");
        File.Copy(sourceLauncher, installedLauncher, overwrite: true);

        if (OperatingSystem.IsMacOS() || OperatingSystem.IsLinux())
        {
            using var process = System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
            {
                FileName = "chmod",
                ArgumentList = { "+x", installedLauncher },
                UseShellExecute = false,
            });
            process?.WaitForExit();
        }

        EnsureUnixPathContains(launcherDir);
    }

    private static void AddDirectoryToUserPath(string directory)
    {
        var existingPath = Environment.GetEnvironmentVariable("PATH", EnvironmentVariableTarget.User) ?? string.Empty;
        var entries = existingPath.Split(';', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
        if (entries.Any(entry => string.Equals(entry, directory, StringComparison.OrdinalIgnoreCase)))
        {
            Log.Information("PATH already contains {Directory}", directory);
            return;
        }

        var updatedPath = string.IsNullOrWhiteSpace(existingPath)
            ? directory
            : $"{directory};{existingPath}";

        Environment.SetEnvironmentVariable("PATH", updatedPath, EnvironmentVariableTarget.User);
        Log.Information("Added {Directory} to the user PATH", directory);
    }

    private static void EnsureUnixPathContains(string directory)
    {
        var shell = Environment.GetEnvironmentVariable("SHELL") ?? "/bin/bash";
        var rcFile = shell.Contains("zsh", StringComparison.OrdinalIgnoreCase)
            ? Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".zshrc")
            : Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".bashrc");

        var exportLine = $"export PATH=\"{directory}:$PATH\"";
        if (File.Exists(rcFile))
        {
            var contents = File.ReadAllText(rcFile);
            if (contents.Contains(directory, StringComparison.Ordinal))
            {
                Log.Information("Shell profile already contains {Directory}", directory);
                return;
            }

            File.AppendAllText(rcFile, Environment.NewLine + exportLine + Environment.NewLine);
            Log.Information("Updated {RcFile} with PATH entry", rcFile);
            return;
        }

        Log.Warning("Could not find shell profile at {RcFile}. Add this line manually: {ExportLine}", rcFile, exportLine);
    }
}
