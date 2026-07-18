using System.Collections.Concurrent;
using System.Text.Json;
using Serilog;

namespace IgniteBT.Build.Unity;

public sealed class UnityBuildSettings
{
    public bool Enabled { get; set; }
    public int MinFilesPerBlob { get; set; } = 4;
    public int MaxFilesPerBlob { get; set; } = 16;
    public int MaxLinesPerBlob { get; set; } = 8000;
    public int MinLinesForUnity { get; set; } = 50;
}

public sealed class UnityBlob
{
    public string OutputPath { get; set; } = string.Empty;
    public List<string> SourceFiles { get; set; } = new();
}

/// <summary>
/// Groups stable translation units into unity blobs for faster compilation.
/// </summary>
public static class UnityBuildPlanner
{
    private static readonly ConcurrentDictionary<string, (long Size, long Mtime, int Lines)> LineCache = new(StringComparer.OrdinalIgnoreCase);
    private static string? _lineCachePath;

    public static void Initialize(string cacheDirectory)
    {
        _lineCachePath = Path.Combine(cacheDirectory, "unity_line_counts.json");
        LoadLineCache();
    }

    public static void SaveLineCache()
    {
        if (string.IsNullOrEmpty(_lineCachePath)) return;
        try
        {
            var serializable = LineCache.ToDictionary(
                kv => kv.Key,
                kv => new LineCountEntry { Size = kv.Value.Size, Mtime = kv.Value.Mtime, Lines = kv.Value.Lines });
            File.WriteAllText(_lineCachePath, JsonSerializer.Serialize(serializable));
        }
        catch (Exception ex)
        {
            Log.Debug(ex, "Failed to save unity line count cache");
        }
    }

    public static List<UnityBlob> Plan(
        IEnumerable<string> sourceFiles,
        IEnumerable<string> excludedFiles,
        UnityBuildSettings settings)
    {
        var excluded = new HashSet<string>(excludedFiles, StringComparer.OrdinalIgnoreCase);
        var eligible = sourceFiles
            .Where(f => !excluded.Contains(f))
            .Select(f => (Path: f, Lines: CountLines(f)))
            .Where(x => x.Lines >= settings.MinLinesForUnity)
            .OrderBy(x => x.Lines)
            .ToList();

        if (!settings.Enabled || eligible.Count < settings.MinFilesPerBlob)
        {
            return sourceFiles
                .Select(f => new UnityBlob { OutputPath = f, SourceFiles = new List<string> { f } })
                .ToList();
        }

        var blobs = new List<UnityBlob>();
        var current = new UnityBlob();
        var currentLines = 0;

        foreach (var (path, lines) in eligible)
        {
            if (current.SourceFiles.Count >= settings.MaxFilesPerBlob
                || currentLines + lines > settings.MaxLinesPerBlob)
            {
                if (current.SourceFiles.Count > 0) blobs.Add(current);
                current = new UnityBlob();
                currentLines = 0;
            }

            current.SourceFiles.Add(path);
            currentLines += lines;
        }

        if (current.SourceFiles.Count > 0) blobs.Add(current);

        foreach (var f in sourceFiles.Where(f => excluded.Contains(f)))
            blobs.Add(new UnityBlob { OutputPath = f, SourceFiles = new List<string> { f } });

        // Small files filtered by MinLinesForUnity must still compile individually.
        var planned = new HashSet<string>(
            blobs.SelectMany(b => b.SourceFiles),
            StringComparer.OrdinalIgnoreCase);
        foreach (var f in sourceFiles.Where(f => !planned.Contains(f)))
            blobs.Add(new UnityBlob { OutputPath = f, SourceFiles = new List<string> { f } });

        return blobs;
    }

    private static int CountLines(string file)
    {
        if (!File.Exists(file)) return 0;

        var info = new FileInfo(file);
        if (LineCache.TryGetValue(file, out var cached)
            && cached.Size == info.Length
            && cached.Mtime == info.LastWriteTimeUtc.Ticks)
            return cached.Lines;

        var lines = File.ReadLines(file).Count();
        LineCache[file] = (info.Length, info.LastWriteTimeUtc.Ticks, lines);
        return lines;
    }

    private static void LoadLineCache()
    {
        if (string.IsNullOrEmpty(_lineCachePath) || !File.Exists(_lineCachePath)) return;
        try
        {
            var loaded = JsonSerializer.Deserialize<Dictionary<string, LineCountEntry>>(File.ReadAllText(_lineCachePath));
            if (loaded == null) return;
            foreach (var (path, entry) in loaded)
                LineCache[path] = (entry.Size, entry.Mtime, entry.Lines);
        }
        catch { /* ignore */ }
    }

    private sealed class LineCountEntry
    {
        public long Size { get; set; }
        public long Mtime { get; set; }
        public int Lines { get; set; }
    }
}
