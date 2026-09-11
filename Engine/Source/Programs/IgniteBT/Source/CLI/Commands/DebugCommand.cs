// ==============================================================================
// WindEffects — IgniteBT — DebugCommand
// Source file for the IgniteBT module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using Serilog;
using IgniteBT.Build.Layout;
using IgniteBT.Build.Compiler;
using IgniteBT.Core.Launcher;
using IgniteBT.Workspace.Modules;

namespace IgniteBT.CLI;

public static class DebugCommand
{
    public static async Task<int> Execute(string[] args)
    {
        var parsed = CommandSchemas.Debug.Parse(args);
        if (!CommandLineHelpers.TryReportErrors(parsed))
        {
            return 1;
        }

        var config = parsed.GetOption("config", "Debug");
        var target = parsed.ResolveTarget("Editor");
        var pidStr = parsed.GetOption("pid", string.Empty);
        var dumpPath = parsed.GetOption("dump", string.Empty);
        var preferCdb = parsed.HasFlag("cdb");
        var explicitArgs = parsed.GetOption("args", string.Empty);

        int? pid = null;
        if (!string.IsNullOrEmpty(pidStr) && int.TryParse(pidStr, out var parsedPid))
        {
            pid = parsedPid;
        }

        Log.Information("Debug Command (WinDbg)");
        if (!string.IsNullOrEmpty(dumpPath))
        {
            Log.Information("Mode: Crash Dump ({DumpPath})", dumpPath);
        }
        else if (pid.HasValue)
        {
            Log.Information("Mode: Attach to Process PID ({PID})", pid.Value);
        }
        else
        {
            Log.Information("Target: {Target}", target);
            Log.Information("Configuration: {Config}", config);
        }

        try
        {
            var location = EngineInstallation.Resolve();
            var buildConfig = CommandLineHelpers.ParseConfiguration(config);
            var layout = new BuildLayout(location.ProjectRoot, CommandLineHelpers.GetCurrentPlatform(), buildConfig);
            var outputRoot = layout.PlatformOutputRoot;

            string? executablePath = null;
            string[] passthroughArgs = Array.Empty<string>();

            if (string.IsNullOrEmpty(dumpPath) && !pid.HasValue)
            {
                var discovery = new ModuleDiscoverer(location.EngineRoot, config, CommandLineHelpers.GetCurrentPlatform());
                var modules = await discovery.DiscoverModulesAsync();
                var outputLayout = new OutputLayout(layout, location.EngineRoot);
                outputLayout.RegisterModules(modules);

                var normalizedTarget = NormalizeRunTarget(target);
                var manifestPath = layout.GetOutputLayoutManifestPath();
                executablePath = OutputLayout.ResolveLaunchExecutable(
                    outputRoot,
                    normalizedTarget,
                    outputLayout.Descriptors,
                    manifestPath);

                if (string.IsNullOrEmpty(executablePath) || !File.Exists(executablePath))
                {
                    Log.Warning("Executable not found for target {Target}. Building first...", normalizedTarget);
                    var buildArgs = new[] { "--target", target, "--config", config };
                    var buildResult = await BuildCommand.Execute(buildArgs);
                    if (buildResult != 0)
                    {
                        return buildResult;
                    }

                    outputLayout.RegisterModules(
                        await new ModuleDiscoverer(location.EngineRoot, config, CommandLineHelpers.GetCurrentPlatform())
                            .DiscoverModulesAsync());
                    executablePath = OutputLayout.ResolveLaunchExecutable(
                        outputRoot,
                        normalizedTarget,
                        outputLayout.Descriptors,
                        layout.GetOutputLayoutManifestPath());
                }

                if (string.IsNullOrEmpty(executablePath) || !File.Exists(executablePath))
                {
                    Log.Error("Executable not found after build for target {Target}", normalizedTarget);
                    return 1;
                }

                passthroughArgs = GetPassthroughArgs(parsed, normalizedTarget, explicitArgs);
            }

            var launchOptions = new WinDbgLaunchOptions
            {
                ExecutablePath = executablePath,
                Arguments = passthroughArgs,
                ProcessId = pid,
                DumpPath = string.IsNullOrEmpty(dumpPath) ? null : dumpPath,
                WorkingDirectory = outputRoot,
                PreferCdb = preferCdb
            };

            return WinDbgResolver.LaunchDebugger(launchOptions);
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Debug command failed");
            return 1;
        }
    }

    private static string NormalizeRunTarget(string target) =>
        target.ToLowerInvariant() switch
        {
            "editor" => "Editor",
            "welauncher" or "launcher" => "WeLauncher",
            "we" or "cli" => "We",
            "crashreporter" or "crash" => "CrashReporter",
            _ => target
        };

    private static string[] GetPassthroughArgs(ParsedCommand parsed, string target, string explicitArgs)
    {
        var result = new List<string>();

        if (!string.IsNullOrWhiteSpace(explicitArgs))
        {
            result.AddRange(explicitArgs.Split(' ', StringSplitOptions.RemoveEmptyEntries));
        }

        var skippedTarget = false;
        foreach (var positional in parsed.Positionals)
        {
            if (!skippedTarget && string.Equals(positional, target, StringComparison.OrdinalIgnoreCase))
            {
                skippedTarget = true;
                continue;
            }

            result.Add(positional);
        }

        return result.ToArray();
    }
}
