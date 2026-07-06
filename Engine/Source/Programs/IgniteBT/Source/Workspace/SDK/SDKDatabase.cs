using System.Collections.Concurrent;
using System.Text.Json;
using Serilog;

namespace IgniteBT.Workspace.SDK;

/// <summary>
/// Manages persistent storage of SDK detection results.
/// </summary>
public class SDKDatabase
{
    private const string CacheFileName = "IgniteBT.SDKCache.json";
    private readonly string[] _searchPaths;
    
    public SDKDatabase()
    {
        _searchPaths = new[]
        {
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
            ".",
            ".."
        };
    }
    
    /// <summary>
    /// Loads cached SDK information from disk.
    /// </summary>
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
                {
                    cache[name] = info;
                }
                
                Log.Information("Loaded {Count} SDKs from cache", cachedData.Count);
            }
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to load SDK cache from: {Path}", cachePath);
        }
    }
    
    /// <summary>
    /// Saves SDK information to disk cache.
    /// </summary>
    public void SaveCache(ConcurrentDictionary<string, SDKInfo> cache)
    {
        var cachePath = GetCacheWritePath();
        if (cachePath == null) return;
        
        try
        {
            var directory = Path.GetDirectoryName(cachePath);
            if (!string.IsNullOrEmpty(directory) && !Directory.Exists(directory))
            {
                Directory.CreateDirectory(directory);
            }
            
            var jsonContent = JsonSerializer.Serialize(cache, new JsonSerializerOptions 
            { 
                WriteIndented = true,
                PropertyNamingPolicy = JsonNamingPolicy.CamelCase
            });
            
            File.WriteAllText(cachePath, jsonContent);
            Log.Information("Saved {Count} SDKs to cache", cache.Count);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to save SDK cache to: {Path}", cachePath);
        }
    }
    
    /// <summary>
    /// Clears the SDK cache.
    /// </summary>
    public void ClearCache()
    {
        var cachePath = FindCachePath();
        if (cachePath != null && File.Exists(cachePath))
        {
            try
            {
                File.Delete(cachePath);
                Log.Information("Cleared SDK cache at: {Path}", cachePath);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Failed to clear SDK cache at: {Path}", cachePath);
            }
        }
    }
    
    /// <summary>
    /// Finds the cache file path for reading.
    /// </summary>
    private string? FindCachePath()
    {
        foreach (var searchPath in _searchPaths)
        {
            var cachePath = Path.Combine(searchPath, CacheFileName);
            if (File.Exists(cachePath))
            {
                return cachePath;
            }
        }
        return null;
    }
    
    /// <summary>
    /// Gets the cache file path for writing.
    /// </summary>
    private string? GetCacheWritePath()
    {
        var localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        if (!string.IsNullOrEmpty(localAppData))
        {
            return Path.Combine(localAppData, CacheFileName);
        }
        return Path.Combine(".", CacheFileName);
    }
}
