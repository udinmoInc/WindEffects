using System.Security.Cryptography;
using System.Text;
using Serilog;
using IgniteBT.Compiler;

namespace IgniteBT.BuildCacheSystem;

/// <summary>
/// Cache entry for a compiled translation unit.
/// </summary>
public class CacheEntry
{
    /// <summary>
    /// Hash of the source file content.
    /// </summary>
    public string SourceHash { get; set; } = string.Empty;

    /// <summary>
    /// Hash of all included headers.
    /// </summary>
    public string HeadersHash { get; set; } = string.Empty;

    /// <summary>
    /// Hash of compiler options.
    /// </summary>
    public string OptionsHash { get; set; } = string.Empty;

    /// <summary>
    /// Hash of compiler version.
    /// </summary>
    public string CompilerHash { get; set; } = string.Empty;

    /// <summary>
    /// Path to the cached object file.
    /// </summary>
    public string ObjectFile { get; set; } = string.Empty;

    /// <summary>
    /// Timestamp when this entry was created.
    /// </summary>
    public DateTime Timestamp { get; set; }

    /// <summary>
    /// Compilation time in milliseconds.
    /// </summary>
    public long CompilationTimeMs { get; set; }
}

/// <summary>
/// Hash-based incremental build cache.
/// </summary>
public class BuildCache
{
    private readonly string _cacheDirectory;
    private readonly Dictionary<string, CacheEntry> _entries = new();
    private readonly string _indexFilePath;

    public BuildCache(string cacheDirectory)
    {
        _cacheDirectory = cacheDirectory;
        _indexFilePath = Path.Combine(cacheDirectory, "cache_index.json");
        
        // Ensure cache directory exists
        Directory.CreateDirectory(cacheDirectory);
        
        // Load existing cache index
        LoadCacheIndex();
    }

    /// <summary>
    /// Computes a hash for a file's content.
    /// </summary>
    public static string ComputeFileHash(string filePath)
    {
        if (!File.Exists(filePath))
        {
            return string.Empty;
        }

        using var sha256 = SHA256.Create();
        using var stream = File.OpenRead(filePath);
        var hashBytes = sha256.ComputeHash(stream);
        return Convert.ToHexString(hashBytes).ToLowerInvariant();
    }

    /// <summary>
    /// Computes a hash for a string.
    /// </summary>
    public static string ComputeStringHash(string content)
    {
        using var sha256 = SHA256.Create();
        var hashBytes = sha256.ComputeHash(Encoding.UTF8.GetBytes(content));
        return Convert.ToHexString(hashBytes).ToLowerInvariant();
    }

    /// <summary>
    /// Computes a combined hash for multiple strings.
    /// </summary>
    public static string ComputeCombinedHash(params string[] values)
    {
        using var sha256 = SHA256.Create();
        foreach (var value in values)
        {
            var bytes = Encoding.UTF8.GetBytes(value);
            sha256.TransformBlock(bytes, 0, bytes.Length, null, 0);
        }
        sha256.TransformFinalBlock(Array.Empty<byte>(), 0, 0);
        return Convert.ToHexString(sha256.Hash ?? Array.Empty<byte>()).ToLowerInvariant();
    }

    /// <summary>
    /// Computes the cache key for a compilation task.
    /// </summary>
    public string ComputeCacheKey(string sourceFile, List<string> includedHeaders, CompilerOptions options, string compilerVersion)
    {
        var sourceHash = ComputeFileHash(sourceFile);
        
        // Compute combined hash of all included headers
        var headerHashes = includedHeaders
            .Select(ComputeFileHash)
            .OrderBy(h => h)
            .ToArray();
        var headersHash = ComputeCombinedHash(headerHashes);
        
        // Hash compiler options
        var optionsString = string.Join("|", new[]
        {
            options.CppStandard,
            options.Configuration.ToString(),
            options.Platform.ToString(),
            options.Architecture.ToString(),
            string.Join(",", options.Definitions.OrderBy(d => d)),
            string.Join(",", options.IncludeDirectories.OrderBy(d => d)),
            string.Join(",", options.AdditionalFlags.OrderBy(d => d)),
            options.GenerateDebugInfo.ToString(),
            options.EnableOptimizations.ToString()
        });
        var optionsHash = ComputeStringHash(optionsString);
        
        // Hash compiler version
        var compilerHash = ComputeStringHash(compilerVersion);
        
        // Combine all hashes
        return ComputeCombinedHash(sourceHash, headersHash, optionsHash, compilerHash);
    }

    /// <summary>
    /// Checks if a cached object file exists and is valid for the given inputs.
    /// </summary>
    public bool TryGetCachedObject(string cacheKey, out string? objectFile)
    {
        if (_entries.TryGetValue(cacheKey, out var entry))
        {
            if (File.Exists(entry.ObjectFile))
            {
                objectFile = entry.ObjectFile;
                Log.Debug("Cache hit for key {Key}: {ObjectFile}", cacheKey[..8], objectFile);
                return true;
            }
            else
            {
                // Cache entry exists but object file is missing - remove it
                _entries.Remove(cacheKey);
                Log.Warning("Cache entry for key {Key} has missing object file, removing", cacheKey[..8]);
            }
        }
        
        objectFile = null;
        Log.Debug("Cache miss for key {Key}", cacheKey[..8]);
        return false;
    }

    /// <summary>
    /// Stores a compiled object file in the cache.
    /// </summary>
    public void CacheObject(string cacheKey, string objectFile, long compilationTimeMs)
    {
        if (!File.Exists(objectFile))
        {
            Log.Warning("Cannot cache non-existent object file: {ObjectFile}", objectFile);
            return;
        }

        // Copy object file to cache directory
        var cachedObjectName = $"{cacheKey}.obj";
        var cachedObjectPath = Path.Combine(_cacheDirectory, cachedObjectName);
        
        try
        {
            File.Copy(objectFile, cachedObjectPath, true);
            
            var entry = new CacheEntry
            {
                SourceHash = cacheKey, // Simplified - in reality we'd store individual hashes
                ObjectFile = cachedObjectPath,
                Timestamp = DateTime.UtcNow,
                CompilationTimeMs = compilationTimeMs
            };
            
            _entries[cacheKey] = entry;
            SaveCacheIndex();
            
            Log.Information("Cached object file for key {Key}: {ObjectFile} ({Time}ms)", cacheKey[..8], cachedObjectName, compilationTimeMs);
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Failed to cache object file {ObjectFile}", objectFile);
        }
    }

    /// <summary>
    /// Invalidates cache entries for a specific source file.
    /// </summary>
    public void InvalidateSource(string sourceFile)
    {
        var sourceHash = ComputeFileHash(sourceFile);
        var keysToRemove = _entries
            .Where(kvp => kvp.Value.SourceHash.StartsWith(sourceHash))
            .Select(kvp => kvp.Key)
            .ToList();
        
        foreach (var key in keysToRemove)
        {
            InvalidateCacheEntry(key);
        }
        
        Log.Information("Invalidated {Count} cache entries for source {Source}", keysToRemove.Count, sourceFile);
    }

    /// <summary>
    /// Invalidates a specific cache entry.
    /// </summary>
    public void InvalidateCacheEntry(string cacheKey)
    {
        if (_entries.TryGetValue(cacheKey, out var entry))
        {
            if (File.Exists(entry.ObjectFile))
            {
                try
                {
                    File.Delete(entry.ObjectFile);
                }
                catch (Exception ex)
                {
                    Log.Warning(ex, "Failed to delete cached object file {ObjectFile}", entry.ObjectFile);
                }
            }
            
            _entries.Remove(cacheKey);
            SaveCacheIndex();
        }
    }

    /// <summary>
    /// Clears all cache entries.
    /// </summary>
    public void Clear()
    {
        foreach (var entry in _entries.Values)
        {
            if (File.Exists(entry.ObjectFile))
            {
                try
                {
                    File.Delete(entry.ObjectFile);
                }
                catch (Exception ex)
                {
                    Log.Warning(ex, "Failed to delete cached object file {ObjectFile}", entry.ObjectFile);
                }
            }
        }
        
        _entries.Clear();
        SaveCacheIndex();
        
        Log.Information("Cleared all cache entries");
    }

    /// <summary>
    /// Gets cache statistics.
    /// </summary>
    public CacheStats GetStats()
    {
        var totalSize = _entries.Values
            .Where(e => File.Exists(e.ObjectFile))
            .Sum(e => new FileInfo(e.ObjectFile).Length);
        
        var stats = new CacheStats
        {
            EntryCount = _entries.Count,
            TotalSizeBytes = totalSize,
            TotalSizeMB = totalSize / (1024.0 * 1024.0)
        };

        if (_entries.Count > 0)
        {
            stats.OldestEntry = _entries.Values.Min(e => e.Timestamp);
            stats.NewestEntry = _entries.Values.Max(e => e.Timestamp);
        }
        else
        {
            stats.OldestEntry = DateTime.MinValue;
            stats.NewestEntry = DateTime.MinValue;
        }

        return stats;
    }

    /// <summary>
    /// Loads the cache index from disk.
    /// </summary>
    private void LoadCacheIndex()
    {
        if (!File.Exists(_indexFilePath))
        {
            return;
        }

        try
        {
            var json = File.ReadAllText(_indexFilePath);
            var entries = System.Text.Json.JsonSerializer.Deserialize<Dictionary<string, CacheEntry>>(json);
            
            if (entries != null)
            {
                _entries.Clear();
                foreach (var kvp in entries)
                {
                    // Only load entries that still have valid object files
                    if (File.Exists(kvp.Value.ObjectFile))
                    {
                        _entries[kvp.Key] = kvp.Value;
                    }
                }
                
                Log.Information("Loaded {Count} cache entries from index", _entries.Count);
            }
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to load cache index, starting with empty cache");
        }
    }

    /// <summary>
    /// Saves the cache index to disk.
    /// </summary>
    private void SaveCacheIndex()
    {
        try
        {
            var json = System.Text.Json.JsonSerializer.Serialize(_entries, new System.Text.Json.JsonSerializerOptions
            {
                WriteIndented = true
            });
            
            File.WriteAllText(_indexFilePath, json);
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Failed to save cache index");
        }
    }
}

/// <summary>
/// Cache statistics.
/// </summary>
public class CacheStats
{
    /// <summary>
    /// Number of cache entries.
    /// </summary>
    public int EntryCount { get; set; }

    /// <summary>
    /// Total size in bytes.
    /// </summary>
    public long TotalSizeBytes { get; set; }

    /// <summary>
    /// Total size in megabytes.
    /// </summary>
    public double TotalSizeMB { get; set; }

    /// <summary>
    /// Oldest cache entry timestamp.
    /// </summary>
    public DateTime OldestEntry { get; set; }

    /// <summary>
    /// Newest cache entry timestamp.
    /// </summary>
    public DateTime NewestEntry { get; set; }
}
