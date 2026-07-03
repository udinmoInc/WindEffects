using Serilog;
using IgniteBT.Build.Layout;
using IgniteBT.Build.Compiler;
using IgniteBT.Core.Launcher;

namespace IgniteBT.CLI;

public static class RunCommand
{
    public static async Task<int> Execute(string[] args)
    {
        var config = GetArgValue(args, "--config", "Debug");
        var target = GetArgValue(args, "--target", "Editor");

        Log.Information("Run Command");
        Log.Information("Target: {Target}", target);
        Log.Information("Configuration: {Config}", config);

        try
        {
            var location = EngineInstallation.Resolve();
            var buildConfig = ParseConfiguration(config);
            var layout = new BuildLayout(location.Descriptor.EngineRoot, GetCurrentPlatform(), buildConfig);
            var outputRoot = layout.PlatformOutputRoot;

            var executableName = target.ToLowerInvariant() switch
            {
                "editor" or "application" => "WindeffectsEditor.exe",
                "crashreporter" => "WECrashReporter.exe",
                "launcher" => "WELauncher.exe",
                _ => target.EndsWith(".exe", StringComparison.OrdinalIgnoreCase) ? target : target + ".exe"
            };

            var executablePath = Path.Combine(outputRoot, executableName);
            if (!File.Exists(executablePath))
            {
                Log.Warning("Executable not found at {Path}. Building first...", executablePath);
                var buildArgs = new[] { target, "--config", config };
                var buildResult = await BuildCommand.Execute(buildArgs);
                if (buildResult != 0)
                {
                    return buildResult;
                }
            }

            if (!File.Exists(executablePath))
            {
                Log.Error("Executable not found after build: {Path}", executablePath);
                return 1;
            }

            Log.Information("Launching {Executable}", executablePath);
            var startInfo = new System.Diagnostics.ProcessStartInfo
            {
                FileName = executablePath,
                WorkingDirectory = outputRoot,
                UseShellExecute = true,
            };

            System.Diagnostics.Process.Start(startInfo);
            return 0;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Run failed");
            return 1;
        }
    }

    private static BuildConfiguration ParseConfiguration(string config) =>
        config.ToLowerInvariant() switch
        {
            "debug" => BuildConfiguration.Debug,
            "development" or "dev" => BuildConfiguration.Development,
            "profile" => BuildConfiguration.Profile,
            "shipping" or "release" => BuildConfiguration.Shipping,
            _ => BuildConfiguration.Debug
        };

    private static string GetArgValue(string[] args, string name, string defaultValue)
    {
        for (var i = 0; i < args.Length; i++)
        {
            if (args[i] == name && i + 1 < args.Length)
            {
                return args[i + 1];
            }
        }

        return defaultValue;
    }

    private static string GetCurrentPlatform()
    {
        if (OperatingSystem.IsWindows()) return "Windows";
        if (OperatingSystem.IsLinux()) return "Linux";
        if (OperatingSystem.IsMacOS()) return "Mac";
        return "Unknown";
    }
}
