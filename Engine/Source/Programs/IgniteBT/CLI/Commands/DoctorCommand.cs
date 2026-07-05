using Serilog;
using IgniteBT.Core.Launcher;
using IgniteBT.Build.Layout;
using IgniteBT.Workspace.SDK;
using IgniteBT.Workspace.ThirdParty;

namespace IgniteBT.CLI;

public static class DoctorCommand
{
    public static async Task<int> Execute(string[] args)
    {
        Log.Information("Doctor Command - Diagnosing WindEffects toolchain");

        var issues = 0;

        try
        {
            var descriptor = BootstrapLauncher.ResolveDescriptor();
            Log.Information("Engine root: {EngineRoot}", descriptor.EngineRoot);
            Log.Information("Engine version: {Version}", descriptor.EngineVersion);
            Log.Information("Programs root: {ProgramsRoot}", descriptor.ProgramsRoot);
            Log.Information("Build root: {BuildRoot}", descriptor.BuildRoot);

            BootstrapManifest.Refresh(descriptor);
            if (BootstrapManifest.TryLoad(descriptor.BootstrapManifestPath, out var manifest))
            {
                Log.Information("Bootstrap manifest: {Path}", descriptor.BootstrapManifestPath);
                Log.Information("Registered tools: {Count}", manifest.Tools.Count);
                foreach (var tool in manifest.Tools.Values.OrderBy(static t => t.Name, StringComparer.OrdinalIgnoreCase))
                {
                    var target = BootstrapManifest.IsToolRunnable(tool)
                        ? tool.ExecutablePath
                        : tool.SourcePath;
                    Log.Information("  - {Name} ({Kind}) -> {Target}", tool.Name, tool.Kind, target);
                }
            }
            else
            {
                Log.Warning("Bootstrap manifest is missing or invalid");
                issues++;
            }
        }
        catch (Exception ex)
        {
            Log.Error("Engine location: FAILED ({Message})", ex.Message);
            issues++;
        }

        if (FindDotNetExecutable() == null)
        {
            Log.Error(".NET SDK: not found on PATH");
            issues++;
        }
        else
        {
            Log.Information(".NET SDK: available");
        }

        SDKManager.Instance.Initialize();
        ThirdPartyManager.Instance.Initialize();

        var engineRoot = BuildLayout.FindEngineRoot(Directory.GetCurrentDirectory());
        if (!string.IsNullOrEmpty(engineRoot))
        {
            if (!await ThirdPartyBootstrapper.EnsureRequiredAsync(engineRoot))
            {
                issues++;
            }
        }

        var sdks = await SDKManager.Instance.DetectAllAsync();
        Log.Information("SDKs detected: {Count}", sdks.Count);

        foreach (var sdk in sdks.Values.OrderBy(s => s.Name, StringComparer.OrdinalIgnoreCase))
        {
            if (sdk.IsValid)
            {
                Log.Information("  [OK] {Name} {Version} -> {Path}", sdk.Name, sdk.Version, sdk.RootPath);
            }
            else
            {
                Log.Warning("  [INVALID] {Name} -> {Path}", sdk.Name, sdk.RootPath);
                issues++;
            }
        }

        if (issues == 0)
        {
            Log.Information("Doctor: no issues found");
            return 0;
        }

        Log.Warning("Doctor: found {Count} issue(s)", issues);
        return 1;
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
