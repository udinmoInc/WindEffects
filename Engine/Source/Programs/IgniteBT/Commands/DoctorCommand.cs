using Serilog;
using IgniteBT.SDK;
using IgniteBT.Launcher;
using IgniteBT.BuildSystem;

namespace IgniteBT.Commands;

public static class DoctorCommand
{
    public static async Task<int> Execute(string[] args)
    {
        Log.Information("Doctor Command - Diagnosing WindEffects toolchain");

        var issues = 0;

        try
        {
            var location = EngineInstallation.Resolve();
            Log.Information("Project root: {ProjectRoot}", location.ProjectRoot);
            Log.Information("Engine root: {EngineRoot}", location.EngineRoot);
        }
        catch (Exception ex)
        {
            Log.Error("Engine location: FAILED ({Message})", ex.Message);
            issues++;
        }

        var igniteBt = TryResolveIgniteBt();
        if (igniteBt == null)
        {
            Log.Warning("IgniteBT binary: not built (will fall back to `dotnet run`)");
        }
        else
        {
            Log.Information("IgniteBT binary: {Path}", igniteBt);
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

    private static string? TryResolveIgniteBt()
    {
        try
        {
            var location = EngineInstallation.Resolve();
            var executable = EngineInstallation.ResolveIgniteBtExecutable(location);
            return string.IsNullOrWhiteSpace(executable) ? null : executable;
        }
        catch
        {
            return null;
        }
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
