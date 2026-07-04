using Serilog;
using IgniteBT.Build.Layout;
using IgniteBT.Build.Compiler;
using IgniteBT.Core.Launcher;
using IgniteBT.Workspace.Modules;

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
            var layout = new BuildLayout(location.ProjectRoot, CommandLineHelpers.GetCurrentPlatform(), buildConfig);
            var outputRoot = layout.PlatformOutputRoot;

            var discovery = new ModuleDiscoverer(location.EngineRoot, config, CommandLineHelpers.GetCurrentPlatform());
            var modules = await discovery.DiscoverModulesAsync();
            var outputLayout = new OutputLayout(layout, location.EngineRoot);
            outputLayout.RegisterModules(modules);

            var normalizedTarget = NormalizeRunTarget(target);
            var executablePath = OutputLayout.ResolveLaunchExecutable(outputRoot, normalizedTarget, outputLayout.Descriptors);

            if (string.IsNullOrEmpty(executablePath) || !File.Exists(executablePath))
            {
                Log.Warning("Executable not found for target {Target}. Building first...", normalizedTarget);
                var buildArgs = BuildLaunchArgs(target, config);
                var buildResult = await BuildCommand.Execute(buildArgs);
                if (buildResult != 0)
                {
                    return buildResult;
                }

                outputLayout.RegisterModules(
                    await new ModuleDiscoverer(location.EngineRoot, config, CommandLineHelpers.GetCurrentPlatform())
                        .DiscoverModulesAsync());
                executablePath = OutputLayout.ResolveLaunchExecutable(outputRoot, normalizedTarget, outputLayout.Descriptors);
            }

            if (string.IsNullOrEmpty(executablePath) || !File.Exists(executablePath))
            {
                Log.Error("Executable not found after build for target {Target}", normalizedTarget);
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

    private static string NormalizeRunTarget(string target) =>
        target.ToLowerInvariant() switch
        {
            "editor" => "Application",
            _ => target
        };

    private static string[] BuildLaunchArgs(string target, string config)
    {
        if (string.Equals(target, "Editor", StringComparison.OrdinalIgnoreCase) ||
            string.Equals(target, "Application", StringComparison.OrdinalIgnoreCase))
        {
            return new[] { "--target", "Application", "--config", config };
        }

        return new[] { target, "--config", config };
    }
}
