using System.Collections.Concurrent;

namespace IgniteBT.Core.Filesystem;

/// <summary>
/// Caches directory listings to avoid repeated filesystem traversal.
/// </summary>
public sealed class DirectoryCache
{
    private readonly ConcurrentDictionary<string, CachedDirectory> _cache = new(StringComparer.OrdinalIgnoreCase);

    public IReadOnlyList<string> GetFiles(string directory, string pattern, SearchOption option = SearchOption.TopDirectoryOnly)
    {
        var key = $"{directory}|{pattern}|{option}";
        return _cache.AddOrUpdate(
            key,
            _ => Scan(directory, pattern, option),
            (_, existing) => existing.IsStale(directory) ? Scan(directory, pattern, option) : existing).Files;
    }

    public void Invalidate(string directory)
    {
        foreach (var key in _cache.Keys.Where(k => k.StartsWith(directory + "|", StringComparison.OrdinalIgnoreCase)))
            _cache.TryRemove(key, out _);
    }

    public void Clear() => _cache.Clear();

    private static CachedDirectory Scan(string directory, string pattern, SearchOption option)
    {
        if (!Directory.Exists(directory))
            return new CachedDirectory(directory, Array.Empty<string>());
        return new CachedDirectory(directory, Directory.GetFiles(directory, pattern, option));
    }

    private sealed class CachedDirectory
    {
        private readonly long _scanTicks;
        public string[] Files { get; }
        public long LastWriteUtcTicks { get; }

        public CachedDirectory(string directory, string[] files)
        {
            Files = files;
            _scanTicks = DateTime.UtcNow.Ticks;
            LastWriteUtcTicks = Directory.Exists(directory)
                ? new DirectoryInfo(directory).LastWriteTimeUtc.Ticks : 0;
        }

        public bool IsStale(string directory)
        {
            if (!Directory.Exists(directory)) return true;
            var current = new DirectoryInfo(directory).LastWriteTimeUtc.Ticks;
            return current != LastWriteUtcTicks || DateTime.UtcNow.Ticks - _scanTicks > TimeSpan.FromSeconds(30).Ticks;
        }
    }
}
