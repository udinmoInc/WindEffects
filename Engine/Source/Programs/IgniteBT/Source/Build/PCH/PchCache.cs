using System.Collections.Concurrent;
using System.Text.Json;
using IgniteBT.Core.Hashing;

namespace IgniteBT.Build.PCH;

public sealed class PchCacheEntry
{
    public string PchPath { get; set; } = string.Empty;
    public string ContentHash { get; set; } = string.Empty;
    public string CompilerHash { get; set; } = string.Empty;
    public DateTime CreatedUtc { get; set; }
}

/// <summary>
/// PCH cache with hash validation and persistence — thread-safe for parallel module compilation.
/// </summary>
public sealed class PchCache
{
    private readonly string _cacheDir;
    private readonly string _indexPath;
    private readonly ConcurrentDictionary<string, PchCacheEntry> _entries = new(StringComparer.OrdinalIgnoreCase);
    private int _dirty;

    public PchCache(string cacheDirectory)
    {
        _cacheDir = Path.Combine(cacheDirectory, "PCH");
        Directory.CreateDirectory(_cacheDir);
        _indexPath = Path.Combine(_cacheDir, "pch_index.json");
        Load();
    }

    public bool TryGetValid(string moduleName, string headerHash, string compilerHash, out string pchPath)
    {
        pchPath = string.Empty;
        if (!_entries.TryGetValue(moduleName, out var entry)) return false;
        if (entry.ContentHash != headerHash || entry.CompilerHash != compilerHash) return false;
        if (!File.Exists(entry.PchPath)) return false;
        pchPath = entry.PchPath;
        return true;
    }

    public void Register(string moduleName, string pchPath, string headerHash, string compilerHash)
    {
        _entries[moduleName] = new PchCacheEntry
        {
            PchPath = pchPath,
            ContentHash = headerHash,
            CompilerHash = compilerHash,
            CreatedUtc = DateTime.UtcNow
        };
        Interlocked.Exchange(ref _dirty, 1);
    }

    public void SaveIfDirty()
    {
        if (Interlocked.CompareExchange(ref _dirty, 0, 1) != 1) return;
        try
        {
            var snapshot = new Dictionary<string, PchCacheEntry>(_entries, StringComparer.OrdinalIgnoreCase);
            File.WriteAllText(_indexPath, JsonSerializer.Serialize(snapshot, new JsonSerializerOptions { WriteIndented = true }));
        }
        catch
        {
            Interlocked.Exchange(ref _dirty, 1);
        }
    }

    public string GetPchPath(string moduleName) => Path.Combine(_cacheDir, $"{moduleName}.pch");

    public int EntryCount => _entries.Count;

    private void Load()
    {
        if (!File.Exists(_indexPath)) return;
        try
        {
            var entries = JsonSerializer.Deserialize<Dictionary<string, PchCacheEntry>>(File.ReadAllText(_indexPath));
            if (entries == null) return;
            foreach (var (key, value) in entries)
            {
                if (File.Exists(value.PchPath))
                    _entries[key] = value;
            }
        }
        catch { /* start fresh */ }
    }
}
