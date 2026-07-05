using System.Collections.Concurrent;
using System.Text.Json;
using Serilog;

namespace IgniteBT.Core.Hashing;

public sealed class FileHashEntry
{
    public string Path { get; set; } = string.Empty;
    public string Hash { get; set; } = string.Empty;
    public long Size { get; set; }
    public long LastWriteUtcTicks { get; set; }
}

/// <summary>
/// Persistent file hash cache. Skips re-hashing when size and mtime are unchanged.
/// </summary>
public sealed class FileHashCache
{
    private readonly string _indexPath;
    private readonly ConcurrentDictionary<string, FileHashEntry> _entries = new(StringComparer.OrdinalIgnoreCase);
    private int _hits;
    private int _misses;

    public FileHashCache(string cacheDirectory)
    {
        Directory.CreateDirectory(cacheDirectory);
        _indexPath = Path.Combine(cacheDirectory, "file_hashes.json");
        Load();
    }

    public string GetHash(string filePath)
    {
        var fullPath = Path.GetFullPath(filePath);
        if (!File.Exists(fullPath))
        {
            return string.Empty;
        }

        var info = new FileInfo(fullPath);
        if (_entries.TryGetValue(fullPath, out var entry)
            && entry.Size == info.Length
            && entry.LastWriteUtcTicks == info.LastWriteTimeUtc.Ticks)
        {
            Interlocked.Increment(ref _hits);
            return entry.Hash;
        }

        Interlocked.Increment(ref _misses);
        var hash = FastHash.HashFile(fullPath);
        _entries[fullPath] = new FileHashEntry
        {
            Path = fullPath,
            Hash = hash,
            Size = info.Length,
            LastWriteUtcTicks = info.LastWriteTimeUtc.Ticks
        };

        return hash;
    }

    public void Invalidate(string filePath) => _entries.TryRemove(Path.GetFullPath(filePath), out _);

    public void Save()
    {
        try
        {
            var json = JsonSerializer.Serialize(_entries, new JsonSerializerOptions { WriteIndented = false });
            File.WriteAllText(_indexPath, json);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to save file hash cache");
        }
    }

    public (int Hits, int Misses, double HitRate) GetStats()
    {
        var total = _hits + _misses;
        return (_hits, _misses, total > 0 ? (double)_hits / total : 0.0);
    }

    private void Load()
    {
        if (!File.Exists(_indexPath)) return;
        try
        {
            var json = File.ReadAllText(_indexPath);
            var loaded = JsonSerializer.Deserialize<Dictionary<string, FileHashEntry>>(json);
            if (loaded == null) return;
            foreach (var (key, value) in loaded) _entries[key] = value;
        }
        catch (Exception ex) { Log.Warning(ex, "Failed to load file hash cache"); }
    }
}
