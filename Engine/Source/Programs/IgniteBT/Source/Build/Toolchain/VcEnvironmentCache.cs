using System.Diagnostics;
using System.Text;
using System.Text.Json;
using Serilog;

namespace IgniteBT.Build.Toolchain;

/// <summary>
/// Captures and reuses MSVC environment from vcvarsall.bat — once per process, optionally persisted to disk.
/// </summary>
public static class VcEnvironmentCache
{
    private static readonly object Lock = new();
    private static Dictionary<string, string>? _memoryCache;
    private static string? _cachedForPath;

    public static Dictionary<string, string>? LoadOrCapture(string? vcVarsAllPath, string? persistDirectory = null)
    {
        if (string.IsNullOrEmpty(vcVarsAllPath) || !File.Exists(vcVarsAllPath))
            return null;

        lock (Lock)
        {
            if (_memoryCache != null && string.Equals(_cachedForPath, vcVarsAllPath, StringComparison.OrdinalIgnoreCase))
                return _memoryCache;

            var vcVarsMtime = File.GetLastWriteTimeUtc(vcVarsAllPath).Ticks;
            if (!string.IsNullOrEmpty(persistDirectory))
            {
                var disk = TryLoadFromDisk(persistDirectory, vcVarsAllPath, vcVarsMtime);
                if (disk != null)
                {
                    _memoryCache = disk;
                    _cachedForPath = vcVarsAllPath;
                    Log.Information("Loaded MSVC environment from disk cache ({Count} variables)", disk.Count);
                    return _memoryCache;
                }
            }

            var captured = CaptureFromVcVars(vcVarsAllPath);
            if (captured == null) return null;

            _memoryCache = captured;
            _cachedForPath = vcVarsAllPath;

            if (!string.IsNullOrEmpty(persistDirectory))
                TrySaveToDisk(persistDirectory, vcVarsAllPath, vcVarsMtime, captured);

            return _memoryCache;
        }
    }

    public static void ApplyToProcessStartInfo(ProcessStartInfo psi, Dictionary<string, string>? environment)
    {
        if (environment == null) return;
        foreach (var (key, value) in environment)
            psi.Environment[key] = value;
    }

    private static Dictionary<string, string>? CaptureFromVcVars(string vcVarsAllPath)
    {
        try
        {
            var script = Path.Combine(Path.GetTempPath(), $"ignitebt_vcenv_{Guid.NewGuid():N}.cmd");
            File.WriteAllText(script, $"""
                @echo off
                call "{vcVarsAllPath}" x64 >nul 2>&1
                set
                """);

            var psi = new ProcessStartInfo
            {
                FileName = "cmd.exe",
                Arguments = $"/C \"{script}\"",
                RedirectStandardOutput = true,
                UseShellExecute = false,
                CreateNoWindow = true,
                StandardOutputEncoding = Encoding.UTF8
            };

            using var proc = Process.Start(psi);
            if (proc == null) return null;

            var env = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            while (proc.StandardOutput.ReadLine() is { } line)
            {
                var eq = line.IndexOf('=');
                if (eq > 0)
                    env[line[..eq]] = line[(eq + 1)..];
            }
            proc.WaitForExit(30000);
            try { File.Delete(script); } catch { /* ignore */ }

            Log.Information("Cached MSVC environment ({Count} variables)", env.Count);
            return env;
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to cache vcvars environment");
            return null;
        }
    }

    private static string GetDiskPath(string persistDirectory) =>
        Path.Combine(persistDirectory, "vc_environment.json");

    private static Dictionary<string, string>? TryLoadFromDisk(string persistDirectory, string vcVarsPath, long vcVarsMtime)
    {
        var path = GetDiskPath(persistDirectory);
        if (!File.Exists(path)) return null;

        try
        {
            var entry = JsonSerializer.Deserialize<VcEnvironmentDiskEntry>(File.ReadAllText(path));
            if (entry?.Environment == null) return null;
            if (!string.Equals(entry.VcVarsPath, vcVarsPath, StringComparison.OrdinalIgnoreCase)) return null;
            if (entry.VcVarsMtimeTicks != vcVarsMtime) return null;
            return entry.Environment;
        }
        catch
        {
            return null;
        }
    }

    private static void TrySaveToDisk(string persistDirectory, string vcVarsPath, long vcVarsMtime, Dictionary<string, string> env)
    {
        try
        {
            Directory.CreateDirectory(persistDirectory);
            var entry = new VcEnvironmentDiskEntry
            {
                VcVarsPath = vcVarsPath,
                VcVarsMtimeTicks = vcVarsMtime,
                Environment = env,
                SavedUtc = DateTime.UtcNow
            };
            File.WriteAllText(GetDiskPath(persistDirectory), JsonSerializer.Serialize(entry));
        }
        catch (Exception ex)
        {
            Log.Debug(ex, "Failed to persist MSVC environment cache");
        }
    }

    private sealed class VcEnvironmentDiskEntry
    {
        public string VcVarsPath { get; set; } = string.Empty;
        public long VcVarsMtimeTicks { get; set; }
        public Dictionary<string, string> Environment { get; set; } = new();
        public DateTime SavedUtc { get; set; }
    }
}
