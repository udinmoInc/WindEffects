using IgniteBT.Build.Toolchain;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Text;
using Serilog;

namespace IgniteBT.Build.Compiler;

/// <summary>
/// Persistent compiler worker pool — warm workers, IPC job queue, crash recovery, idle timeout.
/// Supports MSVC, clang-cl, Clang, and GCC via compiler-specific backends.
/// </summary>
public sealed class CompilerWorkerPool : IDisposable
{
    private readonly ConcurrentBag<CompilerDaemonWorker> _workers = new();
    private readonly SemaphoreSlim _slots;
    private readonly string _compilerPath;
    private readonly string? _vcVarsAllPath;
    private readonly CompilerType _compilerType;
    private readonly Dictionary<string, string>? _cachedEnvironment;
    private readonly Timer _healthTimer;
    private volatile bool _disposed;

    public int ActiveWorkers => _workers.Count;
    public int MaxWorkers { get; }

    public CompilerWorkerPool(
        string compilerPath,
        int maxWorkers,
        string? vcVarsAllPath = null,
        CompilerType compilerType = CompilerType.MSVC)
    {
        _compilerPath = compilerPath;
        _vcVarsAllPath = vcVarsAllPath;
        _compilerType = compilerType;
        MaxWorkers = maxWorkers;
        _slots = new SemaphoreSlim(maxWorkers, maxWorkers);
        _cachedEnvironment = VcEnvironmentCache.LoadOrCapture(vcVarsAllPath);
        _healthTimer = new Timer(HealthCheckCallback, null, TimeSpan.FromMinutes(1), TimeSpan.FromMinutes(1));
        WarmWorkers(Math.Min(maxWorkers, 2));
    }

    private void WarmWorkers(int count)
    {
        for (var i = 0; i < count; i++)
        {
            var worker = CreateWorker();
            _workers.Add(worker);
            _ = worker.EnsureStartedAsync();
        }
    }

    public async Task<CompilationResult> CompileAsync(
        CompilerOptions options,
        bool captureDependencies = false,
        string? dependencyOutputPath = null,
        CancellationToken cancellationToken = default)
    {
        await _slots.WaitAsync(cancellationToken);
        CompilerDaemonWorker? worker = null;
        try
        {
            worker = AcquireWorker();
            await worker.EnsureStartedAsync(cancellationToken);

            var arguments = BuildArguments(options, captureDependencies, dependencyOutputPath);
            var request = new CompilerDaemonRequest
            {
                CompilerExecutable = _compilerPath,
                Arguments = arguments,
                WorkingDirectory = string.IsNullOrEmpty(options.WorkingDirectory)
                    ? Environment.CurrentDirectory
                    : options.WorkingDirectory,
                CaptureDependencies = captureDependencies,
                DependencyOutputPath = dependencyOutputPath ?? string.Empty
            };

            var response = await worker.ExecuteAsync(request, cancellationToken);
            return new CompilationResult
            {
                Success = response.Success,
                OutputFile = options.OutputFile,
                CompilationTimeMs = response.ElapsedMs,
                StandardOutput = response.StandardOutput,
                StandardError = response.StandardError,
                ExitCode = response.ExitCode
            };
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Compiler worker pool compile failed for {Source}", options.SourceFile);
            return new CompilationResult
            {
                Success = false,
                OutputFile = options.OutputFile,
                StandardError = ex.Message
            };
        }
        finally
        {
            if (worker != null) ReleaseWorker(worker);
            _slots.Release();
        }
    }

    private CompilerDaemonWorker AcquireWorker()
    {
        if (_workers.TryTake(out var worker) && !worker.IsIdleExpired())
            return worker;

        var fresh = CreateWorker();
        _workers.Add(fresh);
        return fresh;
    }

    private void ReleaseWorker(CompilerDaemonWorker worker)
    {
        if (!worker.IsAlive || worker.IsIdleExpired())
        {
            worker.Dispose();
            return;
        }
        _workers.Add(worker);
    }

    private CompilerDaemonWorker CreateWorker() =>
        new(_compilerPath, _vcVarsAllPath, _cachedEnvironment);

    private string BuildArguments(CompilerOptions options, bool captureDependencies, string? depOutput)
    {
        var args = CompilerArgumentsBuilder.Build(_compilerType, options);

        if (captureDependencies)
        {
            args = _compilerType switch
            {
                CompilerType.MSVC => args + " /showIncludes",
                CompilerType.Clang => args + $" -MD -MF \"{depOutput}\"",
                CompilerType.GCC => args + $" -MMD -MF \"{depOutput}\"",
                _ => args + " /showIncludes"
            };
        }

        return args;
    }

    private void HealthCheckCallback(object? _)
    {
        if (_disposed) return;
        var stale = new List<CompilerDaemonWorker>();
        while (_workers.TryTake(out var worker))
        {
            if (worker.IsIdleExpired() || !worker.IsAlive)
                stale.Add(worker);
            else
                _workers.Add(worker);
        }
        foreach (var w in stale)
        {
            Log.Debug("Disposing idle/dead compiler worker {Id}", w.WorkerId);
            w.Dispose();
        }
    }

    public CompilerPoolHealth GetHealth()
    {
        var workers = _workers.ToArray();
        return new CompilerPoolHealth
        {
            MaxWorkers = MaxWorkers,
            WarmWorkers = workers.Length,
            AliveWorkers = workers.Count(w => w.IsAlive),
            TotalJobsProcessed = workers.Sum(w => w.JobsProcessed),
            EnvironmentCached = _cachedEnvironment != null
        };
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        _healthTimer.Dispose();
        while (_workers.TryTake(out var worker))
            worker.Dispose();
        _slots.Dispose();
    }
}

public sealed class CompilerPoolHealth
{
    public int MaxWorkers { get; init; }
    public int WarmWorkers { get; init; }
    public int AliveWorkers { get; init; }
    public int TotalJobsProcessed { get; init; }
    public bool EnvironmentCached { get; init; }
}
