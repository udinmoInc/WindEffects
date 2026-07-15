using System.Collections.Concurrent;
using System.Text.Json;
using Serilog;

namespace IgniteBT.Build.Unity;

/// <summary>
/// Adaptive unity filter — excludes recently modified, heavy, and unstable files from unity blobs.
/// Shared per build instance; <see cref="RecordBuild"/> is invoked from parallel compile workers.
/// </summary>
public sealed class AdaptiveUnityFilter
{
    private readonly ConcurrentDictionary<string, DateTime> _lastModified = new(StringComparer.OrdinalIgnoreCase);
    private readonly TimeSpan _stabilityWindow;
    private readonly string? _persistPath;
    private int _dirty;

    public AdaptiveUnityFilter(TimeSpan? stabilityWindow = null, string? databaseDirectory = null)
    {
        _stabilityWindow = stabilityWindow ?? TimeSpan.FromHours(24);
        if (!string.IsNullOrEmpty(databaseDirectory))
        {
            _persistPath = Path.Combine(databaseDirectory, "unity_stability.json");
            Load();
        }
    }

    public void RecordBuild(string sourceFile)
    {
        if (!File.Exists(sourceFile)) return;
        _lastModified[sourceFile] = File.GetLastWriteTimeUtc(sourceFile);
        Interlocked.Exchange(ref _dirty, 1);
    }

    public void SaveIfDirty()
    {
        if (Interlocked.CompareExchange(ref _dirty, 0, 1) != 1
            || string.IsNullOrEmpty(_persistPath))
            return;

        try
        {
            var snapshot = new Dictionary<string, DateTime>(_lastModified, StringComparer.OrdinalIgnoreCase);
            File.WriteAllText(_persistPath, JsonSerializer.Serialize(snapshot));
        }
        catch (Exception ex)
        {
            Interlocked.Exchange(ref _dirty, 1);
            Log.Debug(ex, "Failed to save unity stability cache");
        }
    }

    public IEnumerable<string> GetUnstableFiles(IEnumerable<string> sourceFiles)
    {
        var cutoff = DateTime.UtcNow - _stabilityWindow;
        foreach (var file in sourceFiles)
        {
            if (!File.Exists(file)) continue;
            var mtime = File.GetLastWriteTimeUtc(file);
            if (mtime > cutoff) yield return file;
            else if (_lastModified.TryGetValue(file, out var last) && mtime > last)
                yield return file;
        }
    }

    private void Load()
    {
        if (string.IsNullOrEmpty(_persistPath) || !File.Exists(_persistPath)) return;
        try
        {
            var loaded = JsonSerializer.Deserialize<Dictionary<string, DateTime>>(File.ReadAllText(_persistPath));
            if (loaded == null) return;
            foreach (var (key, value) in loaded)
                _lastModified[key] = value;
        }
        catch { /* ignore corrupt cache */ }
    }
}
