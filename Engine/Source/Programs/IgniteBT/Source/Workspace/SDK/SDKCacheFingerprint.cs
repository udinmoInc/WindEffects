using System.Text;
using IgniteBT.Core.Hashing;
using Microsoft.Win32;
using Serilog;

namespace IgniteBT.Workspace.SDK;

/// <summary>
/// Fingerprints SDK detection inputs — invalidates cache when registry, config, or toolchain changes.
/// </summary>
public static class SDKCacheFingerprint
{
    private const string FingerprintFileName = "sdk_cache_fingerprint.txt";

    public static string Compute(string? compilerPath = null)
    {
        var sb = new StringBuilder();

        foreach (var configPath in FindConfigPaths())
        {
            if (File.Exists(configPath))
            {
                var info = new FileInfo(configPath);
                sb.Append($"cfg:{configPath}:{info.Length}:{info.LastWriteTimeUtc.Ticks}|");
            }
        }

        sb.Append($"compiler:{compilerPath ?? Environment.GetEnvironmentVariable("VCINSTALLDIR") ?? ""}|");
        sb.Append($"vs:{ReadRegistryFingerprint()}|");
        sb.Append($"winsdk:{ReadWindowsSdkFingerprint()}|");
        sb.Append($"vulkan:{Environment.GetEnvironmentVariable("VULKAN_SDK") ?? ""}|");

        return FastHash.HashString(sb.ToString());
    }

    public static bool IsCacheValid(string fingerprint, string? compilerPath = null)
    {
        var stored = LoadStoredFingerprint();
        if (string.IsNullOrEmpty(stored)) return false;
        return string.Equals(stored, Compute(compilerPath), StringComparison.Ordinal);
    }

    public static void SaveFingerprint(string fingerprint)
    {
        try
        {
            var path = GetFingerprintPath();
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            File.WriteAllText(path, fingerprint);
        }
        catch (Exception ex)
        {
            Log.Debug(ex, "Failed to save SDK cache fingerprint");
        }
    }

    private static string LoadStoredFingerprint()
    {
        var path = GetFingerprintPath();
        return File.Exists(path) ? File.ReadAllText(path).Trim() : string.Empty;
    }

    private static string GetFingerprintPath()
    {
        var localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        return Path.Combine(localAppData, FingerprintFileName);
    }

    private static IEnumerable<string> FindConfigPaths()
    {
        yield return Path.Combine(".", "IgniteBT.SDKs.json");
        yield return Path.Combine("..", "IgniteBT.SDKs.json");
        yield return Path.Combine("../..", "IgniteBT.SDKs.json");
    }

    private static string ReadRegistryFingerprint()
    {
        if (!OperatingSystem.IsWindows()) return string.Empty;

        try
        {
            using var key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\VisualStudio\SxS\VS7");
            var value = key?.GetValue("17.0")?.ToString() ?? string.Empty;
            return value;
        }
        catch
        {
            return string.Empty;
        }
    }

    private static string ReadWindowsSdkFingerprint()
    {
        if (!OperatingSystem.IsWindows()) return string.Empty;

        try
        {
            using var key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\Windows\v10.0");
            return key?.GetValue("InstallationFolder")?.ToString() ?? string.Empty;
        }
        catch
        {
            return string.Empty;
        }
    }
}
