using Serilog;
using IgniteBT.Build.Compiler;
using IgniteBT.Build.Layout;
using IgniteBT.Build.Toolchain;

namespace IgniteBT.Daemon;

/// <summary>
/// Persistent IgniteBT daemon — warm caches, file watcher, and compiler worker pool.
/// </summary>
public sealed class IgniteBTDaemon : IDisposable
{
    private readonly string _buildRoot;
    private readonly CompilerWorkerPool? _compilerPool;
    private FileSystemWatcher? _watcher;
    private volatile bool _running;

    public bool IsCompilerPoolWarm => _compilerPool?.ActiveWorkers > 0;

    public IgniteBTDaemon(string projectRoot)
    {
        _buildRoot = Path.Combine(projectRoot, "Build");
        var compiler = ToolchainDetector.DetectCompiler();
        if (compiler.Type != CompilerType.None)
        {
            _compilerPool = new CompilerWorkerPool(
                compiler.Path, Environment.ProcessorCount, compiler.VcVarsAllPath, compiler.Type);
        }
    }

    public void Start()
    {
        if (_running) return;
        _running = true;

        var engineDir = BuildLayout.FindEngineRoot(Directory.GetCurrentDirectory()) ?? Directory.GetCurrentDirectory();
        _watcher = new FileSystemWatcher(engineDir)
        {
            IncludeSubdirectories = true,
            NotifyFilter = NotifyFilters.FileName | NotifyFilters.LastWrite | NotifyFilters.Size,
            Filter = "*.*"
        };
        _watcher.Changed += OnFileChanged;
        _watcher.Created += OnFileChanged;
        _watcher.EnableRaisingEvents = true;

        Log.Information("IgniteBT daemon started — watching {Dir}, compiler pool: {Pool}",
            engineDir, _compilerPool != null ? "warm" : "unavailable");
    }

    private void OnFileChanged(object sender, FileSystemEventArgs e)
    {
        if (e.FullPath.EndsWith(".cpp", StringComparison.OrdinalIgnoreCase)
            || e.FullPath.EndsWith(".h", StringComparison.OrdinalIgnoreCase)
            || e.FullPath.EndsWith(".Build.cs", StringComparison.OrdinalIgnoreCase))
            Log.Debug("Daemon: file change detected {Path}", e.FullPath);
    }

    public CompilerPoolHealth? GetCompilerHealth() => _compilerPool?.GetHealth();

    public void Stop()
    {
        _running = false;
        if (_watcher != null)
        {
            _watcher.EnableRaisingEvents = false;
            _watcher.Dispose();
            _watcher = null;
        }
    }

    public void Dispose()
    {
        Stop();
        _compilerPool?.Dispose();
    }
}
