using System.Collections.Concurrent;
using System.Text.Json;
using IgniteBT.Core.Launcher;
using Serilog;

namespace IgniteBT.Workspace.SDK;

/// <summary>
/// Manages persistent storage of SDK detection results.
/// Prefers project-local Build/Database on the repo drive over %LocalAppData% on C:.
/// </summary>
public class SDKDatabase
{
    private const string CacheFileName = "IgniteBT.SDKCache.json";
    private readonly string[] _searchPaths;
    private readonly string _writePath;

    public SDKDatabase()
    {
        var projectDatabase = BuildEnvironment.ResolveProjectDatabaseDirectory();
        _writePath = Path.Combine(projectDatabase, CacheFileName);
        _searchPaths = [projectDatabase];
    }

    public void LoadCache(ConcurrentDictionary<string, SDKInfo> cache)
    {
        var cachePath = FindCachePath();
        if (cachePath == null || !File.Exists(cachePath))
        {
            Log.Debug("No SDK cache file found");
            return;
        }

        try
        {
            var jsonContent = File.ReadAllText(cachePath);
            var cachedData = JsonSerializer.Deserialize<Dictionary<string, SDKInfo>>(jsonContent,
                new JsonSerializerOptions { PropertyNameCaseInsensitive = true });

            if (cachedData != null)
            {
                foreach (var (name, info) in cachedData)
                    cache[name] = info;

                Log.Information("Loaded {Count} SDKs from cache ({Path})", cachedData.Count, cachePath);
            }
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to load SDK cache from: {Path}", cachePath);
        }
    }

    public void SaveCache(ConcurrentDictionary<string, SDKInfo> cache)
    {
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(_writePath)!);
            var jsonContent = JsonSerializer.Serialize(cache, new JsonSerializerOptions
            {
                WriteIndented = true,
                PropertyNamingPolicy = JsonNamingPolicy.CamelCase
            });

            File.WriteAllText(_writePath, jsonContent);
            Log.Information("Saved {Count} SDKs to cache ({Path})", cache.Count, _writePath);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to save SDK cache to: {Path}", _writePath);
        }
    }

    public void ClearCache()
    {
        var paths = _searchPaths
            .Select(p => Path.Combine(p, CacheFileName))
            .ToList();

        // Remove legacy cache on C: from older IgniteBT builds.
        var legacyProfileAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        var legacy = Path.Combine(legacyProfileAppData, CacheFileName);
        if (!paths.Contains(legacy, StringComparer.OrdinalIgnoreCase))
            paths.Add(legacy);

        foreach (var path in paths.Distinct(StringComparer.OrdinalIgnoreCase))
        {
            if (!File.Exists(path)) continue;
            try
            {
                File.Delete(path);
                Log.Information("Cleared SDK cache at: {Path}", path);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Failed to clear SDK cache at: {Path}", path);
            }
        }
    }

    private string? FindCachePath()
    {
        foreach (var searchPath in _searchPaths)
        {
            var cachePath = Path.Combine(searchPath, CacheFileName);
            if (File.Exists(cachePath))
                return cachePath;
        }
        return null;
    }

}
