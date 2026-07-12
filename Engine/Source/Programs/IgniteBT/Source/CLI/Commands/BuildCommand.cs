using Serilog;
using IgniteBT.Workspace.Modules;
using IgniteBT.Build.Layout;
using IgniteBT.Build.Dependencies;
using IgniteBT.Core.Cache;
using IgniteBT.Build.Toolchain;
using IgniteBT.Build.Shaders;
using IgniteBT.Build.Icons;
using IgniteBT.Workspace.ThirdParty;
using IgniteBT.Build.Orchestration;
using IgniteBT.Build.Compiler;
using IgniteBT.Core.Hashing;
using IgniteBT.Core.Profiling;

namespace IgniteBT.CLI;

public static class BuildCommand
{
    /// <summary>
    /// Ultra-fast no-op probe — no SDK, compiler, shaders, SQLite, or orchestrator.
    /// Returns exit code when no-op, null when a full build is required.
    /// </summary>
    public static int? TryFastNoOp(string[] args)
    {
        var parsed = CommandSchemas.Build.Parse(args);
        if (!parsed.IsValid) return null;
        if (parsed.HasFlag("clean")) return null;

        var buildTarget = parsed.GetOption("target", parsed.ResolveTarget());
        var config = parsed.GetOption("config", "Release");
        var platform = CommandLineHelpers.NormalizePlatform(parsed.GetOption("platform", string.Empty));
        var unityBuild = parsed.HasFlag("unity");
        int unitySize = 0;
        if (int.TryParse(parsed.GetOption("unity-size", ""), out var us)) unitySize = us;
        var unityDisabled = parsed.GetOption("unity-disable", "")
            .Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
        var unityDisabledStr = string.Join(",",
            unityDisabled.OrderBy(s => s, StringComparer.OrdinalIgnoreCase));

        int jobs;
        try { jobs = CommandLineHelpers.ParseJobs(parsed.GetOption("jobs", string.Empty)); }
        catch (ArgumentException) { return null; }

        var currentDir = Directory.GetCurrentDirectory();
        var projectRoot = BuildLayout.FindProjectRoot(currentDir);
        var engineDir = BuildLayout.FindEngineRoot(currentDir);
        if (string.IsNullOrEmpty(projectRoot) || string.IsNullOrEmpty(engineDir) || !Directory.Exists(engineDir))
            return null;

        var buildConfig = CommandLineHelpers.ParseConfiguration(config);
        var layout = BuildLayout.Resolve(currentDir, platform, buildConfig);
        var buildFlagsHash = BuildFlagsHasher.Compute(buildTarget, unityBuild, unitySize, unityDisabled, jobs);

        var probe = FastNoOpProbe.TryProbe(
            engineDir, layout, buildConfig.ToString(), platform, buildTarget, buildFlagsHash,
            jobs, unityBuild, unitySize, unityDisabledStr);

        if (!probe.IsNoOp) return null;

        var profile = Environment.GetEnvironmentVariable("IGNITEBT_PROFILE_NOOP");
        if (!string.IsNullOrEmpty(profile))
        {
            Console.WriteLine(
                $"No-op build - nothing changed ({probe.ElapsedMs}ms: manifest={probe.ManifestLoadMs}ms snapshot={probe.SnapshotLoadMs}ms validate={probe.ValidateMs}ms)");
        }
        else
        {
            Console.WriteLine($"No-op build - nothing changed ({probe.ElapsedMs}ms)");
        }

        return 0;
    }

    public static async Task<int> Execute(string[] args)
    {
        var fast = TryFastNoOp(args);
        if (fast.HasValue) return fast.Value;

        var parsed = CommandSchemas.Build.Parse(args);
        if (!CommandLineHelpers.TryReportErrors(parsed)) return 1;

        var target = parsed.ResolveTarget();
        var buildTarget = parsed.GetOption("target", target);
        var config = parsed.GetOption("config", "Release");
        var platform = CommandLineHelpers.NormalizePlatform(parsed.GetOption("platform", string.Empty));
        var unityBuild = parsed.HasFlag("unity");
        int unitySize = 0;
        if (int.TryParse(parsed.GetOption("unity-size", ""), out var us)) unitySize = us;
        var unityDisabled = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var mod in parsed.GetOption("unity-disable", "").Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
            unityDisabled.Add(mod);

        int jobs;
        try { jobs = CommandLineHelpers.ParseJobs(parsed.GetOption("jobs", string.Empty)); }
        catch (ArgumentException ex) { Log.Error("{Message}", ex.Message); return 1; }

        Log.Information("Build: target={Target} buildTarget={BuildTarget} config={Config} platform={Platform} jobs={Jobs} unity={Unity}",
            target, buildTarget, config, platform, jobs, unityBuild);

        try
        {
            var profiler = new BuildProfiler();
            var currentDir = Directory.GetCurrentDirectory();
            var projectRoot = BuildLayout.FindProjectRoot(currentDir);
            var engineDir = BuildLayout.FindEngineRoot(currentDir);

            if (string.IsNullOrEmpty(projectRoot) || string.IsNullOrEmpty(engineDir) || !Directory.Exists(engineDir))
            {
                Log.Error("Could not find project root from {Dir}", currentDir);
                return 1;
            }

            bool thirdPartyOk;
            using (profiler.Scope(BuildStages.ThirdPartyDiscovery))
                thirdPartyOk = await ThirdPartyBootstrapper.EnsureRequiredAsync(engineDir, jobs);
            if (!thirdPartyOk) return 1;

            DetectedCompiler detectedCompiler;
            using (profiler.Scope(BuildStages.ToolchainDetection))
                detectedCompiler = ToolchainDetector.DetectCompiler();
            Log.Information("Compiler: {Type} v{Version}", detectedCompiler.Type, detectedCompiler.Version);

            DependencyResolutionResult dependencyResult;
            using (profiler.Scope(BuildStages.SdkDetection))
            {
                var dependencyResolver = new DependencyResolver();
                dependencyResult = await dependencyResolver.ResolveAsync(detectedCompiler.Path);
            }
            foreach (var w in dependencyResult.Warnings) Log.Warning(w);
            foreach (var e in dependencyResult.Errors) Log.Error(e);
            if (!dependencyResult.Success) return 1;

            var buildConfig = CommandLineHelpers.ParseConfiguration(config);
            var layout = BuildLayout.Resolve(currentDir, platform, buildConfig);
            layout.EnsureDirectories();

            var outputLayout = new OutputLayout(layout, engineDir);
            using (profiler.Scope(BuildStages.ShaderCompilation))
            {
                var shaderStats = ShaderBytecodeCompiler.CompileAndStage(engineDir, outputLayout.ConfigurationRoot, layout.CacheDirectory);
                if (shaderStats.Skipped > 0)
                    profiler.RecordShaderCacheHit(shaderStats.Skipped);
            }

            ICompiler probeCompiler = detectedCompiler.Type switch
            {
                CompilerType.GCC => new GCCCompiler(),
                CompilerType.Clang => new ClangCompiler(),
                _ => new MSVCCompiler()
            };
            var compilerVersion = detectedCompiler.Type != CompilerType.None && probeCompiler.IsAvailable()
                ? await probeCompiler.DetectVersionAsync()
                : detectedCompiler.Version;

            var cache = new BuildCache(layout.CacheDirectory);

            using var orchestrator = new BuildOrchestrator(new BuildContext
            {
                EngineDir = engineDir,
                Platform = platform,
                Config = buildConfig,
                Jobs = jobs,
                Layout = layout,
                OutputLayout = outputLayout,
                Cache = cache,
                DetectedCompiler = detectedCompiler,
                DependencyResult = dependencyResult,
                CompilerVersion = compilerVersion,
                EnableUnityBuild = unityBuild,
                UnitySize = unitySize,
                UnityDisabledModules = unityDisabled,
                TargetName = buildTarget,
                Profiler = profiler
            });

            var result = await orchestrator.ExecuteAsync();

            var profile = result.Profile ?? orchestrator.Profiler.GetReport();
            var stats = result.Statistics ?? orchestrator.Statistics;
            var cacheStats = cache.GetStats();

            if (result.WasNoOp)
            {
                Log.Information("No-op build - nothing changed ({Ms}ms)", profile.TotalBuildMs);
                return 0;
            }

            Log.Information("=== Build Profile ===");
            Log.Information("Total: {Ms}ms | Critical path: {CpName} ({CpMs}ms) | Cache hit: {CacheHit:F1}% | CPU util: {Cpu:F1}%",
                profile.TotalBuildMs, profile.CriticalPathName, profile.CriticalPathMs, profile.CacheHitPercent, profile.CpuUtilizationPercent);
            Log.Information("Hash: {Hash}ms | Scan: {Scan}ms | Compiler wait: {Cw}ms | Link wait: {Lw}ms | Link cache: {LinkCache:F1}% | Scheduler idle: {Idle}ms",
                profile.HashTimeMs, profile.ScanTimeMs, profile.CompilerWaitMs, profile.LinkWaitMs, profile.LinkCacheHitPercent, stats.SchedulerIdleMs);
            if (profile.ShaderCacheHits > 0)
                Log.Information("Shader cache hits: {Count}", profile.ShaderCacheHits);
            if (stats.ModulesLinkSkipped > 0)
                Log.Information("Links skipped (cache): {Count}", stats.ModulesLinkSkipped);
            foreach (var scope in profile.Scopes.Take(10))
                Log.Information("  {Name}: {Total}ms ({Count}x, avg {Avg:F1}ms)", scope.Name, scope.TotalMs, scope.Count, scope.AverageMs);

            Log.Information("=== Cache Stats ===");
            Log.Information("Object cache hit rate: {Rate:P1} ({Hits}/{Total})",
                stats.ObjectCacheHitRate, stats.ObjectCacheHits, stats.ObjectCacheHits + stats.ObjectCacheMisses);
            Log.Information("Header cache hit rate: {Rate:P1}", stats.HeaderCacheHitRate);
            Log.Information("Files compiled: {Compiled}, skipped: {Skipped}", stats.FilesCompiled, stats.FilesSkipped);
            Log.Information("Object cache entries: {Count} ({Size:F1} MB)", cacheStats.EntryCount, cacheStats.TotalSizeMB);

            if (result.Success)
            {
                using (profiler.Scope(BuildStages.AssetCompilation))
                {
                    var iconStats = IconAtlasCompiler.CompileAndStage(projectRoot, engineDir, outputLayout.ConfigurationRoot, layout.CacheDirectory);
                    if (iconStats.Skipped > 0)
                        profiler.RecordShaderCacheHit(iconStats.Skipped);
                }

                using (profiler.Scope(BuildStages.RuntimeDependencyStaging))
                {
                    outputLayout.PruneConfigurationRoot(result.Modules);
                    outputLayout.RefreshLayoutManifest(result.Modules);
                    outputLayout.StageRuntimeDependencies(dependencyResult, result.Modules);
                }

                var flamePath = Path.Combine(layout.CacheDirectory, "flamegraph.json");
                File.WriteAllText(flamePath, result.FlameGraphJson ?? profiler.ExportFlameGraphJson());
                Log.Information("Flame graph: {Path}", flamePath);
            }

            return result.Success ? 0 : 1;
        }
        catch (ModuleDiscoveryException ex)
        {
            Log.Error(ex, "Module discovery failed");
            return 1;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Build failed");
            return 1;
        }
    }
}
