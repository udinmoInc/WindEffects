using System.Collections.Concurrent;
using Serilog;
using IgniteBT.Build.Compiler;
using IgniteBT.Build.Dependencies;
using IgniteBT.Build.Graph;
using IgniteBT.Build.Layout;
using IgniteBT.Build.Linker;
using IgniteBT.Build.PCH;
using IgniteBT.Build.Scheduler;
using IgniteBT.Build.Toolchain;
using IgniteBT.Build.Unity;
using IgniteBT.Core.Cache;
using IgniteBT.Core.Database;
using IgniteBT.Core.Filesystem;
using IgniteBT.Core.Hashing;
using IgniteBT.Core.Profiling;
using IgniteBT.Core.Serialization;
using IgniteBT.Core.Threading;
using IgniteBT.Workspace.Modules;

namespace IgniteBT.Build.Orchestration;

public sealed class BuildOrchestrator : IDisposable
{
    private readonly BuildContext _ctx;
    private readonly FileHashCache _fileHashes;
    private readonly DirectoryCache _directoryCache;
    private readonly PathNormalizer _pathNormalizer;
    private readonly BuildDb _buildDb;
    private readonly BuildProfiler _profiler;
    private readonly BuildStatistics _stats = new();
    private readonly CompilerWorkerPool? _compilerPool;
    private readonly LinkerWorkerPool? _linkerPool;
    private readonly ObjectCasStore _casStore;
    private readonly BuildAutoTuner _autoTuner;
    private readonly SmartJobPrioritizer _prioritizer = new();
    private readonly PchManager? _pchManager;
    private readonly LinkerCache _linkerCache;
    private readonly ExportDefinitionCache _exportCache;
    private readonly AdaptiveUnityFilter _adaptiveUnity;
    private readonly string _linkerVersion;
    private readonly ConcurrentDictionary<string, List<string>> _moduleObjects = new();

    public BuildOrchestrator(BuildContext ctx)
    {
        _ctx = ctx;
        _fileHashes = new FileHashCache(ctx.Layout.CacheDirectory);
        _directoryCache = new DirectoryCache();
        _pathNormalizer = new PathNormalizer();
        _buildDb = new BuildDb(ctx.Layout.DatabaseDirectory);
        _profiler = ctx.Profiler ?? new BuildProfiler();
        _casStore = new ObjectCasStore(ctx.Layout.CacheDirectory);
        _autoTuner = new BuildAutoTuner(ctx.Layout.DatabaseDirectory);
        _linkerCache = new LinkerCache(ctx.Layout.CacheDirectory);
        _exportCache = new ExportDefinitionCache(ctx.Layout.CacheDirectory);
        _adaptiveUnity = new AdaptiveUnityFilter(databaseDirectory: ctx.Layout.DatabaseDirectory);
        UnityBuildPlanner.Initialize(ctx.Layout.CacheDirectory);
        _linkerVersion = ctx.CompilerVersion;

        if (ctx.DetectedCompiler.Type != CompilerType.None)
        {
            _compilerPool = new CompilerWorkerPool(
                ctx.DetectedCompiler.Path,
                _autoTuner.GetRecommendedJobs(ctx.Jobs),
                ctx.DetectedCompiler.VcVarsAllPath,
                ctx.DetectedCompiler.Type);

            _buildDb.SetCompilerInfo(
                ctx.DetectedCompiler.Type.ToString(),
                ctx.CompilerVersion,
                ctx.DetectedCompiler.Path);

            _pchManager = new PchManager(
                ctx.Layout.CacheDirectory,
                ctx.CompilerVersion,
                _compilerPool,
                CreateCompiler());

            var linker = CreateLinker();
            if (linker.IsAvailable())
            {
                var linkWorkers = Math.Max(2, ctx.Jobs);
                _linkerPool = new LinkerWorkerPool(
                    linker, linkWorkers, ctx.DetectedCompiler.VcVarsAllPath, ctx.Layout.CacheDirectory);
            }
        }
    }

    public BuildStatistics Statistics => _stats;
    public BuildProfiler Profiler => _profiler;

    public async Task<BuildOrchestratorResult> ExecuteAsync()
    {
        using var _ = _profiler.Scope(BuildStages.TotalBuild);

        var featureFlagsHash = FastHash.HashString(string.Join("|",
            _ctx.DependencyResult.FeatureFlags.OrderBy(kv => kv.Key).Select(kv => $"{kv.Key}={kv.Value}")));

        List<DiscoveredModule> modules;
        using (_profiler.Scope(BuildStages.ModuleDiscovery))
            modules = await DiscoverModulesCachedAsync(featureFlagsHash);

        if (modules.Count == 0)
            return new BuildOrchestratorResult { Success = true, Modules = modules };

        var configHash = GraphSerializer.ComputeConfigHash(
            _ctx.Config.ToString(), _ctx.Platform, _ctx.CompilerVersion, featureFlagsHash);

        var graph = new DependencyGraph();
        using (_profiler.Scope(BuildStages.DependencyGraph))
        {
            graph.BuildFromModules(modules, _fileHashes, _directoryCache);
            if (!graph.Validate()) return new BuildOrchestratorResult { Success = false, Modules = modules };
        }
        _ctx.Graph = graph;

        if (!string.IsNullOrEmpty(_ctx.TargetName))
        {
            modules = TargetResolver.FilterModules(modules, graph, _ctx.TargetName, _ctx.UnityDisabledModules);
            graph = new DependencyGraph();
            graph.BuildFromModules(modules, _fileHashes, _directoryCache);
            _ctx.Graph = graph;
        }

        var graphHash = graph.ComputeGraphHash();
        var manifestPath = BuildManifest.GetPath(_ctx.Layout.ManifestDirectory);
        BuildManifest.TryLoad(manifestPath, out var previousManifest);

        var moduleHashInputs = modules.Select(m => (
            m.Name, m.BuildCsPath, m.ModuleDirectory, DiscoverSourceFiles(m))).ToList();
        var currentModuleHashes = NoOpBuildDetector.ComputeModuleHashes(moduleHashInputs, _fileHashes);

        var toolchainHash = FastHash.HashString(_ctx.CompilerVersion);
        if (NoOpBuildDetector.IsNoOpRequired(previousManifest, configHash,
            toolchainHash, graphHash, currentModuleHashes))
        {
            SaveWorkspaceSnapshot(moduleHashInputs, configHash, toolchainHash);
            Log.Information("No-op build - nothing changed ({Elapsed}ms)", _profiler.GetReport().TotalBuildMs);
            _stats.ModulesSkipped = modules.Count;
            return new BuildOrchestratorResult { Success = true, Modules = modules, WasNoOp = true };
        }

        _ctx.OutputLayout.RegisterModules(modules);
        _ctx.OutputLayout.PrepareModuleDirectories(modules);
        using (_profiler.Scope(BuildStages.AssetStaging))
            _ctx.OutputLayout.StageEngineAssets(modules);

        GraphSerializer.Save(Path.Combine(_ctx.Layout.DatabaseDirectory, "dependency_graph.json"), graph, configHash);

        var jobGraph = new JobGraph();
        var unitySettings = BuildUnitySettings();
        var unityDir = Path.Combine(_ctx.Layout.PlatformIntermediateRoot, "Unity");
        var depsDir = Path.Combine(_ctx.Layout.CacheDirectory, "Deps");
        var blobIndex = 0;

        foreach (var node in graph.GetBuildOrder())
        {
            var sources = DiscoverSourceFiles(node.Module);
            var unstable = _adaptiveUnity.GetUnstableFiles(sources).ToHashSet(StringComparer.OrdinalIgnoreCase);
            if (_ctx.UnityDisabledModules?.Contains(node.Name, StringComparer.OrdinalIgnoreCase) == true)
                unstable.UnionWith(sources);
            unstable.UnionWith(sources.Where(f => _buildDb.GetAverageCompileTime(f) > 5000));

            var blobs = UnityBuildPlanner.Plan(sources, unstable, unitySettings);
            var compileJobIds = new List<string>();

            foreach (var blob in blobs)
            {
                var compileSource = unitySettings.Enabled && blob.SourceFiles.Count > 1
                    ? UnityBlobGenerator.Generate(blob, unityDir, blobIndex++)
                    : blob.SourceFiles[0];
                blob.OutputPath = compileSource;
                var jobId = $"compile:{node.Name}:{Path.GetFileName(compileSource)}";
                compileJobIds.Add(jobId);
                _prioritizer.SetHistoricalCompileTime(jobId, _buildDb.GetAverageCompileTime(compileSource));
                _prioritizer.SetDependencyDepth(jobId, node.Dependencies.Count);

                var n = node; var b = blob; var s = compileSource;
                jobGraph.Add(new JobNode
                {
                    Id = jobId,
                    Name = $"Compile {node.Name}/{Path.GetFileName(compileSource)}",
                    Work = async _ => await CompileBlobAsync(n, b, s, depsDir)
                });
            }

            var linkId = $"link:{node.Name}";
            var ln = node;
            var linkNode = new JobNode { Id = linkId, Name = $"Link {node.Name}", Work = async _ => await LinkModuleAsync(ln, graph) };
            linkNode.Dependencies.AddRange(compileJobIds);
            linkNode.Dependencies.AddRange(node.Dependencies.Select(d => $"link:{d.Name}"));
            jobGraph.Add(linkNode);
        }

        bool success;
        using (_profiler.Scope(BuildStages.JobGraphExecution))
        {
            using var scheduler = new JobGraphScheduler(jobGraph, _ctx.Jobs, _prioritizer);
            success = await scheduler.ExecuteAsync();
            _stats.SchedulerIdleMs = scheduler.IdleMs;
            _profiler.RecordSchedulerStall(scheduler.IdleMs);
        }

        if (success)
        {
            using (_profiler.Scope(BuildStages.ManifestWriting))
                SaveManifest(manifestPath, configHash, FastHash.HashString(_ctx.CompilerVersion), graphHash, currentModuleHashes, moduleHashInputs);
        }

        _adaptiveUnity.SaveIfDirty();
        UnityBuildPlanner.SaveLineCache();
        _linkerCache.Save();
        _exportCache.Save();
        _ctx.Cache.SaveIfDirty();
        _casStore.SaveIfDirty();
        _pchManager?.Cache.SaveIfDirty();
        RecordTuningObservation();
        _fileHashes.Save();
        _buildDb.Save();

        return new BuildOrchestratorResult
        {
            Success = success, Modules = modules, Profile = _profiler.GetReport(),
            Statistics = _stats, FlameGraphJson = _profiler.ExportFlameGraphJson()
        };
    }

    private UnityBuildSettings BuildUnitySettings() => new()
    {
        Enabled = _ctx.EnableUnityBuild,
        MaxFilesPerBlob = _ctx.UnitySize > 0 ? _ctx.UnitySize : _autoTuner.GetRecommendedUnitySize(),
        MinFilesPerBlob = Math.Max(2, (_ctx.UnitySize > 0 ? _ctx.UnitySize : _autoTuner.GetRecommendedUnitySize()) / 4)
    };

    private async Task CompileBlobAsync(BuildNode node, UnityBlob blob, string primarySource, string depsDir)
    {
        var objectDir = _ctx.Layout.GetModuleObjectsDirectory(node.Name);
        Directory.CreateDirectory(objectDir);
        var objectFile = Path.Combine(objectDir, Path.GetFileNameWithoutExtension(primarySource) + ".obj");
        var includeDirs = ModuleCompileEnvironment.CollectIncludeDirectories(
            node, _ctx.EngineDir, node.Module.ModuleDirectory, _ctx.DependencyResult, _ctx.Graph!);

        var compileOptions = ModuleCompileEnvironment.CreateCompileOptions(
            node, _ctx.Config, primarySource, objectFile, includeDirs, _ctx.DependencyResult, node.Module.Definitions);

        if (_pchManager != null)
        {
            var pch = await _pchManager.EnsureModulePchAsync(node, includeDirs, _ctx.Config);
            if (pch != null)
            {
                PchManager.ApplyPchToOptions(compileOptions, pch);
            }
        }

        List<string> headers;
        using (_profiler.Scope(BuildStages.HeaderScan))
        {
            var scanSw = System.Diagnostics.Stopwatch.StartNew();
            if (_buildDb.TryGetIncludeGraph(primarySource, out var cachedHeaders))
            { headers = cachedHeaders; _stats.RecordHeaderCacheHit(); _profiler.RecordCacheHit(); }
            else if (_compilerPool != null)
            {
                var scanner = new CompilerDependencyScanner(_compilerPool, _ctx.DetectedCompiler.Type, includeDirs, depsDir);
                headers = (await scanner.ScanFileAsync(primarySource, compileOptions)).IncludedHeaders;
                _buildDb.SetIncludeGraph(primarySource, headers);
                _stats.RecordHeaderCacheMiss(); _profiler.RecordCacheMiss();
            }
            else
            {
                var depScanner = ModuleCompileEnvironment.CreateConfiguredScanner(includeDirs);
                headers = (await depScanner.ScanFileAsync(primarySource, transitive: true)).IncludedHeaders;
                _buildDb.SetIncludeGraph(primarySource, headers);
                _stats.RecordHeaderCacheMiss();
            }
            _profiler.RecordScanTime(scanSw.ElapsedMilliseconds);
        }

        var sdkHash = FastHash.HashString(string.Join("|", _ctx.DependencyResult.SDKs.Values.Select(s => s.Version)));
        var casKey = _casStore.ComputeCasKey(primarySource, headers, compileOptions, _ctx.CompilerVersion, sdkHash);

        if (_casStore.TryGet(casKey, out var casObject))
        {
            using (_profiler.Scope(BuildStages.CacheLookup))
                File.Copy(casObject, objectFile, true);
            _stats.RecordObjectCacheHit(); _stats.RecordFileSkipped(); _profiler.RecordCacheHit();
            _buildDb.IncrementCacheStat("object", true);
        }
        else
        {
            var cacheKey = _ctx.Cache.ComputeCacheKey(primarySource, headers, compileOptions, _ctx.CompilerVersion);
            if (_ctx.Cache.TryGetCachedObject(cacheKey, out var cachedObj) && !string.IsNullOrEmpty(cachedObj))
            {
                using (_profiler.Scope(BuildStages.CacheLookup))
                    File.Copy(cachedObj, objectFile, true);
                _stats.RecordObjectCacheHit(); _stats.RecordFileSkipped(); _profiler.RecordCacheHit();
            }
            else
            {
                using (_profiler.Scope(BuildStages.Compile))
                {
                    var compileSw = System.Diagnostics.Stopwatch.StartNew();
                    var result = _compilerPool != null
                        ? await _compilerPool.CompileAsync(compileOptions)
                        : await CreateCompiler().CompileAsync(compileOptions);
                    if (!result.Success) {
                        var details = string.Join(
                            Environment.NewLine,
                            new[] { result.StandardError, result.StandardOutput }
                                .Where(text => !string.IsNullOrWhiteSpace(text)));
                        throw new InvalidOperationException(
                            string.IsNullOrWhiteSpace(details)
                                ? $"Compilation failed for {primarySource} (exit {result.ExitCode})"
                                : $"Compilation failed: {details}");
                    }
                    compileSw.Stop();
                    _profiler.RecordCompilerWait(compileSw.ElapsedMilliseconds);
                    _profiler.RecordTranslationUnitTiming(primarySource, compileSw.ElapsedMilliseconds);
                    _buildDb.RecordCompileTime(primarySource, node.Name, compileSw.ElapsedMilliseconds);
                    _ctx.Cache.CacheObject(cacheKey, primarySource, headers, compileOptions, _ctx.CompilerVersion, objectFile, result.CompilationTimeMs);
                    _casStore.Store(casKey, objectFile, result.CompilationTimeMs);
                    _stats.RecordObjectCacheMiss(); _stats.RecordFileCompiled();
                    _stats.RecordSlowCompile(primarySource, compileSw.ElapsedMilliseconds);
                    _profiler.RecordCacheMiss();
                    _buildDb.IncrementCacheStat("object", false);
                }
            }
        }

        _adaptiveUnity.RecordBuild(primarySource);

        _moduleObjects.AddOrUpdate(node.Name, _ => new List<string> { objectFile },
            (_, list) => { lock (list) { list.Add(objectFile); } return list; });
        _profiler.RecordModuleTiming(node.Name, 0);
    }

    private async Task LinkModuleAsync(BuildNode node, DependencyGraph graph)
    {
        using var _ = _profiler.Scope(BuildStages.Link);

        if (!_moduleObjects.TryGetValue(node.Name, out var objectFiles))
            throw new InvalidOperationException($"No objects for module {node.Name}");
        List<string> snapshot; lock (objectFiles) snapshot = objectFiles.ToList();
        var linker = CreateLinker();
        if (!linker.IsAvailable()) return;
        _ctx.OutputLayout.EnsureModuleDirectories(node.Module);
        var linkOutput = _ctx.OutputLayout.GetModuleBinaryPath(node.Module);
        Directory.CreateDirectory(Path.GetDirectoryName(linkOutput)!);
        var linkOptions = ModuleLinkEnvironment.CreateLinkOptions(
            node, snapshot, linkOutput, _ctx.Config, _ctx.Platform,
            _ctx.Layout, _ctx.OutputLayout, _ctx.DependencyResult, graph, _ctx.EngineDir);

        var objectHashes = snapshot.Select(_fileHashes.GetHash).Where(h => !string.IsNullOrEmpty(h)).ToList();
        foreach (var libDir in linkOptions.LibraryDirectories)
        {
            foreach (var lib in linkOptions.Libraries)
            {
                var libPath = Path.Combine(libDir, lib);
                if (!File.Exists(libPath)) continue;
                var libHash = _fileHashes.GetHash(libPath);
                if (!string.IsNullOrEmpty(libHash)) objectHashes.Add(libHash);
            }
        }

        var exportHash = linkOptions.ExportAllSymbols
            ? FastHash.CombineHashes(objectHashes.OrderBy(h => h, StringComparer.Ordinal).ToArray()) : null;
        var linkKey = _linkerCache.ComputeLinkKey(linkOptions, objectHashes, _linkerVersion, exportHash);

        if (_linkerCache.TryGetHit(linkKey, out var cachedLink))
        {
            _stats.RecordLinkCacheHit();
            _stats.RecordModuleLinkSkipped();
            _profiler.RecordLinkCacheHit();
            Log.Debug("Skipped link for {Module} — inputs unchanged", node.Name);
            return;
        }

        _stats.RecordLinkCacheMiss();
        _profiler.RecordLinkCacheMiss();

        var linkSw = System.Diagnostics.Stopwatch.StartNew();
        var result = linkOptions.TargetType == LinkTargetType.StaticLibrary
            ? await linker.CreateStaticLibraryAsync(linkOptions)
            : _linkerPool != null ? await _linkerPool.LinkAsync(linkOptions) : await linker.LinkAsync(linkOptions);
        linkSw.Stop();
        _profiler.RecordLinkWait(linkSw.ElapsedMilliseconds);
        if (!result.Success) throw new InvalidOperationException($"Link failed: {result.StandardError}");

        var outputHash = FastHash.HashFile(linkOutput);
        _linkerCache.Store(linkKey, linkOptions, outputHash);
        _stats.RecordModuleBuilt();
    }

    private void RecordTuningObservation()
    {
        var report = _profiler.GetReport();
        var activeMs = report.Scopes.Sum(s => s.TotalMs);
        _autoTuner.RecordBuild(new BuildTuningObservation
        {
            TotalBuildMs = report.TotalBuildMs,
            CpuUtilization = report.TotalBuildMs > 0 ? (double)activeMs / report.TotalBuildMs : 0,
            ObjectCacheHitRate = _stats.ObjectCacheHitRate,
            PchReuseRate = _stats.PchReuses > 0 ? 1.0 : 0,
            UnityEnabled = _ctx.EnableUnityBuild,
            UnitySize = _ctx.UnitySize > 0 ? _ctx.UnitySize : _autoTuner.GetRecommendedUnitySize()
        });
    }

    private async Task<List<DiscoveredModule>> DiscoverModulesCachedAsync(string featureFlagsHash)
    {
        var cache = new ModuleDiscoveryCache(_ctx.Layout.DatabaseDirectory, _fileHashes, _directoryCache);
        if (cache.TryLoad(_ctx.EngineDir, _ctx.Config.ToString(), _ctx.Platform, featureFlagsHash, out var cached, out var stale))
        {
            if (stale.Count == 0) return cached;
            var fresh = await CreateDiscoverer().DiscoverModulesAsync(stale);
            var merged = cached.Concat(fresh).ToList();
            cache.Save(_ctx.EngineDir, _ctx.Config.ToString(), _ctx.Platform, featureFlagsHash, merged);
            return merged;
        }
        var full = await CreateDiscoverer().DiscoverModulesAsync();
        cache.Save(_ctx.EngineDir, _ctx.Config.ToString(), _ctx.Platform, featureFlagsHash, full);
        return full;
    }

    private ModuleDiscoverer CreateDiscoverer()
    {
        var d = new ModuleDiscoverer(_ctx.EngineDir, _ctx.Config.ToString(), _ctx.Platform);
        d.SetDependencyResult(_ctx.DependencyResult);
        return d;
    }

    private List<string> DiscoverSourceFiles(DiscoveredModule module)
    {
        if (module.SourceFiles.Count > 0) return module.SourceFiles.Select(_pathNormalizer.Normalize).ToList();
        var dir = module.ModuleDirectory;
        return _directoryCache.GetFiles(dir, "*.cpp", SearchOption.AllDirectories)
            .Concat(_directoryCache.GetFiles(dir, "*.cxx", SearchOption.AllDirectories))
            .Concat(_directoryCache.GetFiles(dir, "*.cc", SearchOption.AllDirectories))
            .Select(_pathNormalizer.Normalize).ToList();
    }

    private ICompiler CreateCompiler()
    {
        ICompiler c = _ctx.DetectedCompiler.Type switch
        {
            CompilerType.GCC => new GCCCompiler(),
            CompilerType.Clang => new ClangCompiler(),
            _ => new MSVCCompiler()
        };
        if (c is MSVCCompiler msvc)
        {
            msvc.SetExecutablePath(_ctx.DetectedCompiler.Path);
            if (!string.IsNullOrEmpty(_ctx.DetectedCompiler.VcVarsAllPath)) msvc.SetVcVarsAllPath(_ctx.DetectedCompiler.VcVarsAllPath);
        }
        return c;
    }

    private ILinker CreateLinker()
    {
        ILinker l = _ctx.DetectedCompiler.Type switch
        {
            CompilerType.GCC => new GNULinker(),
            CompilerType.Clang => new LLDLinker(),
            _ => new MSVCLinker()
        };
        if (l is MSVCLinker msvc && !string.IsNullOrEmpty(_ctx.DetectedCompiler.Path))
        {
            var linkPath = Path.Combine(Path.GetDirectoryName(_ctx.DetectedCompiler.Path)!, "link.exe");
            if (File.Exists(linkPath)) msvc.SetExecutablePath(linkPath);
            if (!string.IsNullOrEmpty(_ctx.DetectedCompiler.VcVarsAllPath))
                msvc.SetVcVarsAllPath(_ctx.DetectedCompiler.VcVarsAllPath);
            var env = VcEnvironmentCache.LoadOrCapture(_ctx.DetectedCompiler.VcVarsAllPath, _ctx.Layout.CacheDirectory);
            if (env != null) msvc.SetEnvironmentVariables(env);
            msvc.SetExportDefinitionCache(_exportCache, _fileHashes);
        }
        return l;
    }

    private void SaveWorkspaceSnapshot(
        List<(string Name, string BuildCsPath, string ModuleDir, List<string> SourceFiles)> moduleHashInputs,
        string manifestConfigHash,
        string manifestToolchainHash)
    {
        var buildCsPaths = moduleHashInputs.Select(m => m.BuildCsPath).ToList();
        var sourcePaths = moduleHashInputs
            .SelectMany(m => m.SourceFiles.Append(m.BuildCsPath))
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToList();

        var featureFlagsHash = FastHash.HashString(string.Join("|",
            _ctx.DependencyResult.FeatureFlags.OrderBy(kv => kv.Key).Select(kv => $"{kv.Key}={kv.Value}")));
        var buildFlagsHash = BuildFlagsHasher.Compute(
            _ctx.TargetName, _ctx.EnableUnityBuild, _ctx.UnitySize,
            _ctx.UnityDisabledModules ?? [], _ctx.Jobs);

        var unityDisabledStr = string.Join(",",
            (_ctx.UnityDisabledModules ?? []).OrderBy(s => s, StringComparer.OrdinalIgnoreCase));

        WorkspaceSnapshot.Create(
            _ctx.Config.ToString(),
            _ctx.Platform,
            _ctx.TargetName,
            buildFlagsHash,
            featureFlagsHash,
            manifestConfigHash,
            manifestToolchainHash,
            _ctx.CompilerVersion,
            _ctx.DetectedCompiler.Path,
            _ctx.Jobs,
            _ctx.EnableUnityBuild,
            _ctx.UnitySize,
            unityDisabledStr,
            buildCsPaths,
            sourcePaths)
            .Save(_ctx.Layout.DatabaseDirectory);
    }

    private void SaveManifest(
        string path,
        string configHash,
        string toolchainHash,
        string graphHash,
        Dictionary<string, string> moduleHashes,
        List<(string Name, string BuildCsPath, string ModuleDir, List<string> SourceFiles)> moduleHashInputs)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        new BuildManifest { ConfigHash = configHash, ToolchainHash = toolchainHash,
            GraphHash = graphHash, ModuleOutputHashes = moduleHashes, BuiltUtc = DateTime.UtcNow }.Save(path);

        SaveWorkspaceSnapshot(moduleHashInputs, configHash, toolchainHash);
    }

    public void Dispose() { _compilerPool?.Dispose(); _linkerPool?.Dispose(); _buildDb.Dispose(); _profiler.Dispose(); }
}

public sealed class BuildOrchestratorResult
{
    public bool Success { get; init; }
    public bool WasNoOp { get; init; }
    public List<DiscoveredModule> Modules { get; init; } = new();
    public BuildProfileReport? Profile { get; init; }
    public BuildStatistics? Statistics { get; init; }
    public string? FlameGraphJson { get; init; }
}

public sealed class BuildContext
{
    public required string EngineDir { get; init; }
    public required string Platform { get; init; }
    public required BuildConfiguration Config { get; init; }
    public required int Jobs { get; init; }
    public required BuildLayout Layout { get; init; }
    public required OutputLayout OutputLayout { get; init; }
    public required BuildCache Cache { get; init; }
    public required DetectedCompiler DetectedCompiler { get; init; }
    public required DependencyResolutionResult DependencyResult { get; init; }
    public required string CompilerVersion { get; init; }
    public DependencyGraph? Graph { get; set; }
    public bool EnableUnityBuild { get; init; }
    public int UnitySize { get; init; }
    public string? TargetName { get; init; }
    public HashSet<string>? UnityDisabledModules { get; init; }
    public BuildProfiler? Profiler { get; init; }
}
