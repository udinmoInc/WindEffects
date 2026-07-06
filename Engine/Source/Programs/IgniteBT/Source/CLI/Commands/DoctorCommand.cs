using Serilog;
using IgniteBT.Build.Compiler;
using IgniteBT.Build.Layout;
using IgniteBT.Build.Toolchain;
using IgniteBT.Core.Cache;
using IgniteBT.Core.Database;
using IgniteBT.Core.Launcher;
using IgniteBT.Core.Threading;
using IgniteBT.Daemon;
using IgniteBT.Workspace.SDK;
using IgniteBT.Workspace.ThirdParty;

namespace IgniteBT.CLI;

public static class DoctorCommand
{
    public static async Task<int> Execute(string[] args)
    {
        Log.Information("=== IgniteBT Doctor ===");
        var issues = 0;

        try
        {
            var descriptor = BootstrapLauncher.ResolveDescriptor();
            Log.Information("Engine root: {EngineRoot}", descriptor.EngineRoot);
            Log.Information("Build root: {BuildRoot}", descriptor.BuildRoot);
            BootstrapManifest.Refresh(descriptor);
        }
        catch (Exception ex)
        {
            Log.Error("Engine location: FAILED ({Message})", ex.Message);
            issues++;
        }

        var layout = BuildLayout.Resolve(Directory.GetCurrentDirectory(), "Win64", BuildConfiguration.Development);

        Log.Information("--- Cache Health ---");
        var cache = new BuildCache(layout.CacheDirectory);
        var cacheStats = cache.GetStats();
        Log.Information("Object cache: {Count} entries, {Size:F1} MB", cacheStats.EntryCount, cacheStats.TotalSizeMB);

        var cacheReg = new PersistentCacheRegistry(layout.CacheDirectory);
        var cacheHealth = cacheReg.GetHealth();
        Log.Information("Cache registry: {Valid}/{Total} valid ({Pct:P1})",
            cacheHealth.ValidEntries, cacheHealth.TotalEntries, cacheHealth.HealthPercent);
        if (cacheHealth.MissingEntries > 0)
        {
            Log.Warning("Cache registry: {Missing} missing entries", cacheHealth.MissingEntries);
            issues++;
        }

        var cas = new ObjectCasStore(layout.CacheDirectory);
        var casStats = cas.GetStats();
        Log.Information("Object CAS: {Count} entries, {Size:F1} MB", casStats.EntryCount, casStats.TotalSizeBytes / (1024.0 * 1024.0));

        Log.Information("--- Database Health ---");
        using (var db = new BuildDb(layout.DatabaseDirectory))
        {
            var health = db.GetHealth();
            Log.Information("SQLite Build.db: {Size:F2} MB | Modules: {Modules} | Objects: {Objects} | Include edges: {Deps} | History: {History}",
                health.DatabaseSizeBytes / (1024.0 * 1024.0), health.ModuleCount, health.ObjectCount, health.HeaderDepCount, health.CompileHistoryCount);
        }

        Log.Information("--- Compiler Workers ---");
        var compiler = ToolchainDetector.DetectCompiler();
        if (compiler.Type != CompilerType.None)
        {
            using var pool = new CompilerWorkerPool(compiler.Path, 2, compiler.VcVarsAllPath, compiler.Type);
            var poolHealth = pool.GetHealth();
            Log.Information("Pool: {Alive}/{Max} alive, env cached: {Env}", poolHealth.AliveWorkers, poolHealth.MaxWorkers, poolHealth.EnvironmentCached);
        }
        else
        {
            Log.Warning("No compiler detected");
            issues++;
        }

        Log.Information("--- Thread Pool ---");
        using (var pool = new WorkStealingThreadPool(4))
        {
            pool.Start();
            var stats = pool.GetStats();
            Log.Information("Workers: {Count}, global queue: {Q}", stats.WorkerCount, stats.GlobalQueueSize);
        }

        Log.Information("--- System ---");
        Log.Information("CPU cores: {Cores}, memory: {Mem:F1} GB",
            Environment.ProcessorCount, GC.GetGCMemoryInfo().TotalAvailableMemoryBytes / (1024.0 * 1024.0 * 1024));

        SDKManager.Instance.Initialize();
        ThirdPartyManager.Instance.Initialize();
        var engineRoot = BuildLayout.FindEngineRoot(Directory.GetCurrentDirectory());
        if (!string.IsNullOrEmpty(engineRoot) && !await ThirdPartyBootstrapper.EnsureRequiredAsync(engineRoot))
            issues++;

        var sdks = await SDKManager.Instance.DetectAllAsync();
        Log.Information("--- SDK Cache ({Count}) ---", sdks.Count);
        foreach (var sdk in sdks.Values.OrderBy(s => s.Name))
        {
            if (!sdk.IsValid) issues++;
            Log.Information("  [{Status}] {Name} {Version}", sdk.IsValid ? "OK" : "INVALID", sdk.Name, sdk.Version);
        }

        Log.Information("--- Daemon Status ---");
        using (var daemon = new IgniteBTDaemon(Directory.GetCurrentDirectory()))
        {
            daemon.Start();
            Log.Information("Daemon: running, compiler pool warm: {Warm}", daemon.IsCompilerPoolWarm);
            daemon.Stop();
        }

        if (issues == 0)
        {
            Log.Information("Doctor: all systems healthy");
            return 0;
        }

        Log.Warning("Doctor: {Count} issue(s) found", issues);
        return 1;
    }
}
