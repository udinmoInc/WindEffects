using System.Diagnostics;
using System.Text.Json;
using IgniteBT.Core.Hashing;
using Serilog;

namespace IgniteBT.Build.Shaders;

/// <summary>
/// Compiles HLSL entry shaders to SPIR-V with incremental caching.
/// </summary>
public static class ShaderBytecodeCompiler
{
    private const string CompileMutexName = @"Global\Windeffects_ShaderCompile";

    private static readonly (string RelativeSource, string ShaderName)[] EntryShaders =
    {
        ("Editor/EditorBackground.hlsl", "EditorBackground"),
        ("Editor/EditorGrid.hlsl", "EditorGrid"),
        ("Foundation/ProceduralSky.hlsl", "ProceduralSky"),
        ("Rendering/AtmospherePass.hlsl", "AtmospherePass"),
        ("Rendering/SceneObject.hlsl", "SceneObject"),
        ("Rendering/TextMSDF.hlsl", "TextMSDF"),
        ("Rendering/UI.hlsl", "UI"),
    };

    public sealed class ShaderCompileStats
    {
        public int Compiled { get; set; }
        public int Skipped { get; set; }
        public long ElapsedMs { get; set; }
    }

    public static ShaderCompileStats CompileAndStage(string engineRoot, string configurationRoot, string? cacheDirectory = null)
    {
        var stats = new ShaderCompileStats();
        var sw = Stopwatch.StartNew();

        var shaderSourceRoot = Path.Combine(engineRoot, "Shaders");
        if (!Directory.Exists(shaderSourceRoot))
        {
            Log.Debug("Shader source root not found: {Path}", shaderSourceRoot);
            return stats;
        }

        var dxcPath = ResolveDxcExecutable();
        if (string.IsNullOrEmpty(dxcPath))
        {
            Log.Warning("DXC not found; shader bytecode will not be compiled. Install the DirectX Shader Compiler.");
            return stats;
        }

        var outputDir = Path.Combine(configurationRoot, "Engine", "Shaders", "Bytecodes");
        Directory.CreateDirectory(outputDir);

        var cache = new ShaderBytecodeCache(cacheDirectory ?? Path.Combine(configurationRoot, "..", "Cache"));
        var dxcVersion = File.GetLastWriteTimeUtc(dxcPath).Ticks.ToString();
        var includeHash = ComputeIncludeTreeHash(shaderSourceRoot);

        using var compileLock = AcquireCompileLock();
        if (compileLock == null)
        {
            Log.Warning("Timed out waiting for shader compile lock; skipping bytecode generation.");
            return stats;
        }

        var tasks = new List<(string Source, string Output, string Profile, string Entry, bool Spirv)>();
        foreach (var (relativeSource, shaderName) in EntryShaders)
        {
            var sourcePath = Path.Combine(shaderSourceRoot, relativeSource);
            if (!File.Exists(sourcePath))
            {
                Log.Warning("Shader source missing: {Path}", sourcePath);
                continue;
            }

            // SPIR-V for Vulkan / OpenGL / Null soft-load
            tasks.Add((sourcePath, Path.Combine(outputDir, $"{shaderName}_VS.spv"), "vs_6_0", "VSMain", true));
            tasks.Add((sourcePath, Path.Combine(outputDir, $"{shaderName}_PS.spv"), "ps_6_0", "PSMain", true));
            // DXIL for DirectX 12 / 11
            tasks.Add((sourcePath, Path.Combine(outputDir, $"{shaderName}_VS.dxil"), "vs_6_0", "VSMain", false));
            tasks.Add((sourcePath, Path.Combine(outputDir, $"{shaderName}_PS.dxil"), "ps_6_0", "PSMain", false));
        }

        var compiled = 0;
        var skipped = 0;

        Parallel.ForEach(tasks, task =>
        {
            var sourceHash = FastHash.HashFile(task.Source);
            var targetTag = task.Spirv ? "spirv" : "dxil";
            var cacheKey = FastHash.CombineHashes(sourceHash, includeHash, dxcVersion, task.Profile, task.Entry, targetTag);

            if (cache.TryGetHit(cacheKey, task.Output))
            {
                Interlocked.Increment(ref skipped);
                return;
            }

            if (CompileStage(dxcPath, shaderSourceRoot, task.Source, task.Output, task.Profile, task.Entry, task.Spirv))
            {
                cache.Store(cacheKey, task.Output, sourceHash, includeHash, dxcVersion, task.Profile, task.Entry);
                Interlocked.Increment(ref compiled);
            }
        });

        stats.Compiled = compiled;
        stats.Skipped = skipped;

        cache.Save();
        sw.Stop();
        stats.ElapsedMs = sw.ElapsedMilliseconds;

        if (stats.Compiled > 0)
            Log.Information("Compiled {Count} shader bytecode files ({Skipped} cached) in {Ms}ms",
                stats.Compiled, stats.Skipped, stats.ElapsedMs);
        else if (stats.Skipped > 0)
            Log.Debug("All {Count} shader bytecode files up to date ({Ms}ms)", stats.Skipped, stats.ElapsedMs);

        return stats;
    }

    private static string ComputeIncludeTreeHash(string shaderRoot)
    {
        if (!Directory.Exists(shaderRoot)) return string.Empty;
        var hashes = Directory.GetFiles(shaderRoot, "*.hlsli", SearchOption.AllDirectories)
            .Concat(Directory.GetFiles(shaderRoot, "*.hlsl", SearchOption.AllDirectories))
            .Select(FastHash.HashFile)
            .OrderBy(h => h, StringComparer.Ordinal)
            .ToArray();
        return FastHash.CombineHashes(hashes);
    }

    private static Mutex? AcquireCompileLock()
    {
        var mutex = new Mutex(false, CompileMutexName);
        if (mutex.WaitOne(TimeSpan.FromMinutes(2)))
            return mutex;

        mutex.Dispose();
        return null;
    }

    private static bool CompileStage(
        string dxcPath,
        string includeRoot,
        string sourcePath,
        string outputPath,
        string profile,
        string entryPoint,
        bool spirv)
    {
        var tempOutputPath = outputPath + ".tmp";
        if (File.Exists(tempOutputPath))
            TryDeleteFile(tempOutputPath);

        var spirvFlag = spirv ? "-spirv " : "";
        var targetDefine = spirv ? "-DWE_TARGET_SPIRV=1 " : "-DWE_TARGET_DXIL=1 ";
        var arguments =
            $"-nologo {spirvFlag}{targetDefine}-T {profile} -E {entryPoint} " +
            $"-I \"{includeRoot}\" \"{sourcePath}\" -Fo \"{tempOutputPath}\"";

        var startInfo = new ProcessStartInfo
        {
            FileName = dxcPath,
            Arguments = arguments,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
        };

        using var process = Process.Start(startInfo);
        if (process == null)
        {
            Log.Error("Failed to start DXC for {Output}", Path.GetFileName(outputPath));
            return false;
        }

        var stdout = process.StandardOutput.ReadToEnd();
        var stderr = process.StandardError.ReadToEnd();
        process.WaitForExit();

        if (process.ExitCode != 0 || !File.Exists(tempOutputPath))
        {
            TryDeleteFile(tempOutputPath);
            Log.Error(
                "DXC failed for {Output} (exit {ExitCode}): {Error}",
                Path.GetFileName(outputPath),
                process.ExitCode,
                string.IsNullOrWhiteSpace(stderr) ? stdout : stderr);
            return false;
        }

        if (!TryReplaceFile(tempOutputPath, outputPath))
        {
            TryDeleteFile(tempOutputPath);
            Log.Error("Failed to publish shader bytecode: {Output}", Path.GetFileName(outputPath));
            return false;
        }

        Log.Debug("Compiled shader bytecode: {Output}", outputPath);
        return true;
    }

    private static bool TryReplaceFile(string sourcePath, string destinationPath, int maxAttempts = 10)
    {
        for (var attempt = 1; attempt <= maxAttempts; attempt++)
        {
            try
            {
                if (File.Exists(destinationPath))
                    File.Replace(sourcePath, destinationPath, null);
                else
                    File.Move(sourcePath, destinationPath);

                return true;
            }
            catch (IOException ex) when (attempt < maxAttempts)
            {
                Log.Debug(ex, "Retrying shader bytecode publish for {Output} (attempt {Attempt}/{MaxAttempts})",
                    Path.GetFileName(destinationPath), attempt, maxAttempts);
                Thread.Sleep(100 * attempt);
            }
        }

        return false;
    }

    private static void TryDeleteFile(string path)
    {
        try
        {
            if (File.Exists(path))
                File.Delete(path);
        }
        catch (IOException ex)
        {
            Log.Debug(ex, "Could not delete temporary shader bytecode: {Path}", path);
        }
    }

    private static string? ResolveDxcExecutable()
    {
        var pathEnv = Environment.GetEnvironmentVariable("PATH");
        if (!string.IsNullOrEmpty(pathEnv))
        {
            foreach (var folder in pathEnv.Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries))
            {
                var candidate = Path.Combine(folder.Trim('"'), "dxc.exe");
                if (File.Exists(candidate))
                    return candidate;
            }
        }

        var localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        var wingetRoot = Path.Combine(
            localAppData,
            "Microsoft",
            "WinGet",
            "Packages",
            "Microsoft.DirectX.ShaderCompiler_Microsoft.Winget.Source_8wekyb3d8bbwe",
            "bin",
            "x64",
            "dxc.exe");
        if (File.Exists(wingetRoot))
            return wingetRoot;

        foreach (var candidate in new[]
                 {
                     @"C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\dxc.exe",
                     @"C:\VulkanSDK\1.3.290.0\Bin\dxc.exe",
                 })
        {
            if (File.Exists(candidate))
                return candidate;
        }

        return null;
    }

    private sealed class ShaderBytecodeCache
    {
        private readonly string _indexPath;
        private readonly Dictionary<string, ShaderCacheEntry> _entries = new(StringComparer.Ordinal);

        public ShaderBytecodeCache(string cacheDirectory)
        {
            Directory.CreateDirectory(cacheDirectory);
            _indexPath = Path.Combine(cacheDirectory, "shader_bytecode_cache.json");
            Load();
        }

        public bool TryGetHit(string cacheKey, string outputPath)
        {
            if (!_entries.TryGetValue(cacheKey, out var entry)) return false;
            if (!File.Exists(outputPath)) return false;
            return string.Equals(FastHash.HashFile(outputPath), entry.OutputHash, StringComparison.Ordinal);
        }

        public void Store(string cacheKey, string outputPath, string sourceHash, string includeHash,
            string dxcVersion, string profile, string entry)
        {
            _entries[cacheKey] = new ShaderCacheEntry
            {
                OutputPath = outputPath,
                OutputHash = FastHash.HashFile(outputPath),
                SourceHash = sourceHash,
                IncludeHash = includeHash,
                DxcVersion = dxcVersion,
                Profile = profile,
                Entry = entry,
                StoredUtc = DateTime.UtcNow
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
                Log.Debug(ex, "Failed to save shader bytecode cache");
            }
        }

        private void Load()
        {
            if (!File.Exists(_indexPath)) return;
            try
            {
                var loaded = JsonSerializer.Deserialize<Dictionary<string, ShaderCacheEntry>>(File.ReadAllText(_indexPath));
                if (loaded == null) return;
                foreach (var (key, entry) in loaded)
                    _entries[key] = entry;
            }
            catch { /* ignore */ }
        }
    }

    private sealed class ShaderCacheEntry
    {
        public string OutputPath { get; set; } = string.Empty;
        public string OutputHash { get; set; } = string.Empty;
        public string SourceHash { get; set; } = string.Empty;
        public string IncludeHash { get; set; } = string.Empty;
        public string DxcVersion { get; set; } = string.Empty;
        public string Profile { get; set; } = string.Empty;
        public string Entry { get; set; } = string.Empty;
        public DateTime StoredUtc { get; set; }
    }
}
