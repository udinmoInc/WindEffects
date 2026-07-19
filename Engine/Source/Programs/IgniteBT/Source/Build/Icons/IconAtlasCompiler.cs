using System.Diagnostics;
using System.Globalization;
using System.Text.Json;
using System.Text.RegularExpressions;
using IgniteBT.Core.Hashing;
using Serilog;

namespace IgniteBT.Build.Icons;

/// <summary>
/// Compiles LibGDX atlas PNG/.atlas source pairs into WindEffects .weiconatlas and icons.weiconmeta assets.
/// Atlas tiers are discovered from ui_Atlas_&lt;N&gt;.* files — never hardcoded.
/// </summary>
public static class IconAtlasCompiler
{
    private static readonly Regex AtlasTierRegex = new(
        @"^ui_Atlas_(\d+)\.(atlas|png|weiconatlas)$",
        RegexOptions.IgnoreCase | RegexOptions.Compiled);

    public sealed class IconCompileStats
    {
        public int Compiled { get; set; }
        public int Skipped { get; set; }
        public long ElapsedMs { get; set; }
    }

    public static IconCompileStats CompileAndStage(
        string projectRoot,
        string engineRoot,
        string configurationRoot,
        string? cacheDirectory = null)
    {
        var stats = new IconCompileStats();
        var sw = Stopwatch.StartNew();

        var inputDir = Path.Combine(projectRoot, "Assets", "Icons", "Atlas");
        if (!Directory.Exists(inputDir))
        {
            Log.Debug("Icon atlas source directory not found: {Path}", inputDir);
            return stats;
        }

        var outputDir = inputDir;
        var tiers = DiscoverAtlasTiers(inputDir);
        if (tiers.Count == 0)
        {
            Log.Debug("No ui_Atlas_<N> sources discovered in {Path}", inputDir);
            return stats;
        }

        var compilerExe = ResolveWeCliExecutable(configurationRoot, engineRoot);
        if (string.IsNullOrEmpty(compilerExe))
        {
            Log.Warning("we.exe not found; icon atlas assets will not be refreshed.");
            return stats;
        }

        var cache = new IconAtlasCompileCache(cacheDirectory ?? Path.Combine(configurationRoot, "..", "Cache"));
        var sourceHash = ComputeSourceTreeHash(inputDir, tiers);
        var outputMeta = Path.Combine(outputDir, "icons.weiconmeta");
        var cacheKey = FastHash.CombineHashes(sourceHash, compilerExe, outputDir);

        if (cache.TryGetHit(cacheKey, outputMeta) && AllOutputsExist(outputDir, tiers))
        {
            stats.Skipped = tiers.Count + 1;
            sw.Stop();
            stats.ElapsedMs = sw.ElapsedMilliseconds;
            Log.Debug("Icon atlas assets up to date ({Ms}ms, {Tiers} tiers)", stats.ElapsedMs, tiers.Count);
            return stats;
        }

        if (!RunCompiler(compilerExe, inputDir, outputDir))
        {
            return stats;
        }

        cache.Store(cacheKey, outputMeta, sourceHash, compilerExe, outputDir);
        cache.Save();

        stats.Compiled = tiers.Count + 1;
        sw.Stop();
        stats.ElapsedMs = sw.ElapsedMilliseconds;
        Log.Information(
            "Compiled icon atlas assets ({Tiers} tiers + meta) in {Ms}ms",
            tiers.Count,
            stats.ElapsedMs);
        return stats;
    }

    public static List<uint> DiscoverAtlasTiers(string inputDir)
    {
        var tiers = new SortedSet<uint>();
        foreach (var file in Directory.EnumerateFiles(inputDir))
        {
            var name = Path.GetFileName(file);
            var match = AtlasTierRegex.Match(name);
            if (!match.Success)
                continue;
            if (uint.TryParse(match.Groups[1].Value, NumberStyles.Integer, CultureInfo.InvariantCulture, out var tier)
                && tier > 0)
            {
                tiers.Add(tier);
            }
        }
        return tiers.ToList();
    }

    private static bool AllOutputsExist(string outputDir, IReadOnlyList<uint> tiers)
    {
        if (!File.Exists(Path.Combine(outputDir, "icons.weiconmeta")))
            return false;

        foreach (var tier in tiers)
        {
            // Only require compiled output for tiers that have a source .atlas.
            var atlasSource = Path.Combine(outputDir, $"ui_Atlas_{tier}.atlas");
            if (!File.Exists(atlasSource))
                continue;
            if (!File.Exists(Path.Combine(outputDir, $"ui_Atlas_{tier}.weiconatlas")))
                return false;
        }

        return true;
    }

    private static string ComputeSourceTreeHash(string inputDir, IReadOnlyList<uint> tiers)
    {
        var hashes = tiers
            .SelectMany(tier => new[]
            {
                Path.Combine(inputDir, $"ui_Atlas_{tier}.png"),
                Path.Combine(inputDir, $"ui_Atlas_{tier}.atlas"),
            })
            .Where(File.Exists)
            .Select(FastHash.HashFile)
            .OrderBy(h => h, StringComparer.Ordinal)
            .ToArray();
        return FastHash.CombineHashes(hashes);
    }

    private static bool RunCompiler(string weCliExe, string inputDir, string outputDir)
    {
        var arguments = $"asset icons --input \"{inputDir}\" --output \"{outputDir}\"";
        var startInfo = new ProcessStartInfo
        {
            FileName = weCliExe,
            Arguments = arguments,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
        };

        using var process = Process.Start(startInfo);
        if (process == null)
        {
            Log.Error("Failed to start we.exe icon compile: {Exe}", weCliExe);
            return false;
        }

        var stdout = process.StandardOutput.ReadToEnd();
        var stderr = process.StandardError.ReadToEnd();
        process.WaitForExit();

        if (process.ExitCode != 0)
        {
            Log.Error(
                "we asset icons failed (exit {ExitCode}): {Error}",
                process.ExitCode,
                string.IsNullOrWhiteSpace(stderr) ? stdout : stderr);
            return false;
        }

        if (!string.IsNullOrWhiteSpace(stdout))
            Log.Information(stdout.TrimEnd());

        return true;
    }

    private static string? ResolveWeCliExecutable(string configurationRoot, string engineRoot)
    {
        var candidates = new[]
        {
            Path.Combine(configurationRoot, "we.exe"),
            Path.Combine(engineRoot, "..", "Build", "Output", "Win64", "Development", "we.exe"),
            Path.Combine(engineRoot, "..", "Build", "Output", "Win64", "Shipping", "we.exe"),
        };

        foreach (var candidate in candidates)
        {
            var fullPath = Path.GetFullPath(candidate);
            if (File.Exists(fullPath))
                return fullPath;
        }

        return null;
    }

    private sealed class IconAtlasCompileCache
    {
        private readonly string _indexPath;
        private readonly Dictionary<string, IconCacheEntry> _entries = new(StringComparer.Ordinal);

        public IconAtlasCompileCache(string cacheDirectory)
        {
            Directory.CreateDirectory(cacheDirectory);
            _indexPath = Path.Combine(cacheDirectory, "icon_atlas_compile_cache.json");
            Load();
        }

        public bool TryGetHit(string cacheKey, string outputMetaPath)
        {
            if (!_entries.TryGetValue(cacheKey, out var entry))
                return false;
            if (!File.Exists(outputMetaPath))
                return false;
            return string.Equals(FastHash.HashFile(outputMetaPath), entry.OutputHash, StringComparison.Ordinal);
        }

        public void Store(string cacheKey, string outputMetaPath, string sourceHash, string compilerExe, string outputDir)
        {
            _entries[cacheKey] = new IconCacheEntry
            {
                OutputPath = outputMetaPath,
                OutputHash = File.Exists(outputMetaPath) ? FastHash.HashFile(outputMetaPath) : string.Empty,
                SourceHash = sourceHash,
                CompilerExe = compilerExe,
                OutputDir = outputDir,
                StoredUtc = DateTime.UtcNow,
            };
        }

        public void Save()
        {
            try
            {
                File.WriteAllText(_indexPath, JsonSerializer.Serialize(_entries));
            }
            catch (Exception ex)
            {
                Log.Debug(ex, "Failed to save icon atlas compile cache");
            }
        }

        private void Load()
        {
            if (!File.Exists(_indexPath))
                return;
            try
            {
                var loaded = JsonSerializer.Deserialize<Dictionary<string, IconCacheEntry>>(File.ReadAllText(_indexPath));
                if (loaded == null)
                    return;
                foreach (var (key, entry) in loaded)
                    _entries[key] = entry;
            }
            catch
            {
                // ignore corrupt cache
            }
        }
    }

    private sealed class IconCacheEntry
    {
        public string OutputPath { get; set; } = string.Empty;
        public string OutputHash { get; set; } = string.Empty;
        public string SourceHash { get; set; } = string.Empty;
        public string CompilerExe { get; set; } = string.Empty;
        public string OutputDir { get; set; } = string.Empty;
        public DateTime StoredUtc { get; set; }
    }
}
