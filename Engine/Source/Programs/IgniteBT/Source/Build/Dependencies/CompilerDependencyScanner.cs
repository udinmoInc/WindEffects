using IgniteBT.Build.Compiler;
using IgniteBT.Build.Toolchain;
using Serilog;

namespace IgniteBT.Build.Dependencies;

/// <summary>
/// Uses compiler-generated dependency information (/showIncludes, -MD, -MMD) for exact include graphs.
/// Falls back to regex scanning only when compiler deps are unavailable.
/// </summary>
public sealed class CompilerDependencyScanner
{
    private readonly CompilerWorkerPool _workerPool;
    private readonly CompilerType _compilerType;
    private readonly DependencyScanner _fallbackScanner;
    private readonly string _depsDirectory;

    public CompilerDependencyScanner(
        CompilerWorkerPool workerPool,
        CompilerType compilerType,
        IEnumerable<string> includeDirectories,
        string depsDirectory)
    {
        _workerPool = workerPool;
        _compilerType = compilerType;
        _fallbackScanner = new DependencyScanner();
        _depsDirectory = depsDirectory;
        Directory.CreateDirectory(_depsDirectory);

        foreach (var path in includeDirectories)
            _fallbackScanner.AddIncludePath(path);
    }

    public async Task<FileDependencies> ScanFileAsync(
        string sourceFile,
        CompilerOptions compileOptions,
        bool transitive = false)
    {
        var deps = new FileDependencies { SourceFile = sourceFile };

        if (!File.Exists(sourceFile))
            return deps;

        var depOutputPath = Path.Combine(_depsDirectory, $"{Path.GetFileNameWithoutExtension(sourceFile)}.d");
        var scanOptions = CloneForDependencyScan(compileOptions, depOutputPath);

        var result = await _workerPool.CompileAsync(scanOptions, captureDependencies: true, depOutputPath);

        List<string> headers;
        if (_compilerType == CompilerType.MSVC)
        {
            headers = ShowIncludesParser.ParseMsvcShowIncludes(result.StandardError);
        }
        else
        {
            headers = ShowIncludesParser.ParseDependencyFile(depOutputPath, sourceFile);
        }

        if (headers.Count == 0)
        {
            Log.Debug("Compiler deps empty for {Source}, falling back to regex scan", sourceFile);
            return await _fallbackScanner.ScanFileAsync(sourceFile, transitive);
        }

        deps.IncludedHeaders = headers;
        deps.DependenciesHash = ComputeHash(headers);
        return deps;
    }

    private static CompilerOptions CloneForDependencyScan(CompilerOptions options, string depOutput)
    {
        return new CompilerOptions
        {
            SourceFile = options.SourceFile,
            OutputFile = Path.Combine(Path.GetDirectoryName(depOutput)!, "_scan.obj"),
            IncludeDirectories = new List<string>(options.IncludeDirectories),
            Definitions = new List<string>(options.Definitions),
            AdditionalFlags = new List<string>(options.AdditionalFlags),
            Configuration = options.Configuration,
            Platform = options.Platform,
            Architecture = options.Architecture,
            CppStandard = options.CppStandard,
            GenerateDebugInfo = false,
            EnableOptimizations = false,
            WorkingDirectory = options.WorkingDirectory
        };
    }

    private static string ComputeHash(List<string> headers)
    {
        var sorted = headers.OrderBy(h => h, StringComparer.OrdinalIgnoreCase);
        return IgniteBT.Core.Hashing.FastHash.CombineHashes(sorted.ToArray());
    }
}
