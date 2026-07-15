using IgniteBT.Build.Compiler;
using IgniteBT.Build.Graph;
using IgniteBT.Core.Hashing;
using IgniteBT.Workspace.Modules;
using Serilog;

namespace IgniteBT.Build.PCH;

public sealed class PchManager
{
    private readonly PchCache _cache;
    private readonly string _compilerVersion;
    private readonly CompilerWorkerPool? _workerPool;
    private readonly ICompiler _compiler;

    public PchManager(string cacheDirectory, string compilerVersion,
        CompilerWorkerPool? workerPool, ICompiler compiler)
    {
        _cache = new PchCache(cacheDirectory);
        _compilerVersion = compilerVersion;
        _workerPool = workerPool;
        _compiler = compiler;
    }

    public PchCache Cache => _cache;

    public async Task<string?> EnsureModulePchAsync(
        BuildNode node, List<string> includeDirectories, BuildConfiguration config, string? sharedPchPath = null)
    {
        var module = node.Module;
        var headerPath = FindDefaultPchHeader(module);
        if (headerPath == null) return sharedPchPath;

        var headerHash = FastHash.HashFile(headerPath);
        var compilerHash = FastHash.HashString(_compilerVersion);
        var flagsHash = FastHash.CombineHashes(config.ToString(),
            FastHash.CombineHashes(includeDirectories.OrderBy(p => p).ToArray()));
        var contentHash = FastHash.CombineHashes(headerHash, compilerHash, flagsHash);

        if (_cache.TryGetValid(module.Name, contentHash, compilerHash, out var existingPch))
        {
            Log.Debug("PCH cache hit for {Module}", module.Name);
            return existingPch;
        }

        var pchOutput = _cache.GetPchPath(module.Name);
        var objectOutput = pchOutput + ".obj";
        var options = new CompilerOptions
        {
            SourceFile = headerPath,
            OutputFile = objectOutput,
            Configuration = config,
            Platform = TargetPlatform.Windows,
            Architecture = TargetArchitecture.x64,
            CppStandard = "c++23",
            IncludeDirectories = includeDirectories,
            IsPrecompiledHeader = true,
            PrecompiledHeader = headerPath,
            GenerateDebugInfo = config == BuildConfiguration.Debug,
            EnableOptimizations = config != BuildConfiguration.Debug
        };

        Log.Information("Generating PCH for {Module}: {Header}", module.Name, headerPath);
        var result = _workerPool != null
            ? await _workerPool.CompileAsync(options)
            : await _compiler.GeneratePrecompiledHeaderAsync(options);

        if (!result.Success)
        {
            Log.Warning("PCH generation failed for {Module}: {Error}", module.Name, result.StandardError);
            return sharedPchPath;
        }

        _cache.Register(module.Name, pchOutput, contentHash, compilerHash);
        return pchOutput;
    }

    public static void ApplyPchToOptions(CompilerOptions options, string? pchPath, bool isFirstInclude = false)
    {
        if (string.IsNullOrEmpty(pchPath)) return;
        options.PrecompiledHeader = pchPath;
        if (!isFirstInclude) options.AdditionalFlags.Add($"/Fp\"{pchPath}\"");
        options.AdditionalFlags.Add($"/FI\"{pchPath}\"");
    }

    private static string? FindDefaultPchHeader(DiscoveredModule module)
    {
        if (!string.IsNullOrWhiteSpace(module.PrecompiledHeader))
        {
            var configured = Path.Combine(module.ModuleDirectory, module.PrecompiledHeader);
            if (File.Exists(configured))
            {
                return configured;
            }
        }

        if (!string.IsNullOrWhiteSpace(module.PrecompiledHeaderSource) && File.Exists(module.PrecompiledHeaderSource))
        {
            return module.PrecompiledHeaderSource;
        }

        var candidates = new[]
        {
            Path.Combine(module.ModuleDirectory, $"{module.Name}PCH.h"),
            Path.Combine(module.ModuleDirectory, "Private", $"{module.Name}PrivatePCH.h")
        };
        return candidates.FirstOrDefault(File.Exists);
    }
}
