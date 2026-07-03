using Serilog;
using IgniteBT.Build.Layout;
using IgniteBT.Build.Compiler;
using IgniteBT.Core.Launcher;

namespace IgniteBT.CLI;

public static class RunCommand
{
    public static async Task<int> Execute(string[] args)
    {
        var parsed = CommandSchemas.Run.Parse(args);
        if (!CommandLineHelpers.TryReportErrors(parsed))
        {
            return 1;
        }

        var config = parsed.GetOption("config", "Debug");
        var target = parsed.ResolveTarget("Editor");

        Log.Information("Run Command");
        Log.Information("Target: {Target}", target);
        Log.Information("Configuration: {Config}", config);

        try
        {
            var location = EngineInstallation.Resolve();
            var buildConfig = CommandLineHelpers.ParseConfiguration(config);
            var layout = new BuildLayout(location.Descriptor.EngineRoot, CommandLineHelpers.GetCurrentPlatform(), buildConfig);
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
                var buildArgs = BuildLaunchArgs(target, config);
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

    private static string[] BuildLaunchArgs(string target, string config)
    {
        if (string.Equals(target, "Editor", StringComparison.OrdinalIgnoreCase) ||
            string.Equals(target, "Application", StringComparison.OrdinalIgnoreCase))
        {
            return new[] { "--target", target, "--config", config };
        }

        return new[] { target, "--config", config };
    }
}
