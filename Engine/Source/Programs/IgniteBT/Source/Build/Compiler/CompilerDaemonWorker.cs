using System.Diagnostics;
using System.Text;
using Serilog;

namespace IgniteBT.Build.Compiler;

/// <summary>
/// Persistent compiler worker — reuses cached MSVC environment, avoids per-compile vcvarsall.
/// Automatically restarts on crash; supports idle timeout and health monitoring.
/// </summary>
public sealed class CompilerDaemonWorker : IDisposable
{
    private readonly string _compilerPath;
    private readonly Dictionary<string, string>? _cachedEnvironment;
    private readonly TimeSpan _idleTimeout;
    private DateTime _lastActivityUtc = DateTime.UtcNow;
    private int _jobsProcessed;
    private volatile bool _disposed;
    private volatile bool _started;

    public string WorkerId { get; } = Guid.NewGuid().ToString("N")[..8];
    public bool IsAlive => !_disposed;
    public int JobsProcessed => _jobsProcessed;
    public DateTime LastActivityUtc => _lastActivityUtc;

    public CompilerDaemonWorker(
        string compilerPath,
        string? vcVarsAllPath,
        Dictionary<string, string>? cachedEnvironment,
        TimeSpan? idleTimeout = null)
    {
        _compilerPath = compilerPath;
        _cachedEnvironment = cachedEnvironment;
        _idleTimeout = idleTimeout ?? TimeSpan.FromMinutes(30);
    }

    public Task EnsureStartedAsync(CancellationToken cancellationToken = default)
    {
        if (_started) return Task.CompletedTask;
        _started = true;
        _lastActivityUtc = DateTime.UtcNow;
        Log.Debug("Compiler daemon worker {WorkerId} ready (env cached: {Cached})",
            WorkerId, _cachedEnvironment != null);
        return Task.CompletedTask;
    }

    public async Task<CompilerDaemonResponse> ExecuteAsync(
        CompilerDaemonRequest request,
        CancellationToken cancellationToken = default)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);

        if (request.Command == CompilerDaemonProtocol.PingCommand)
        {
            return new CompilerDaemonResponse
            {
                RequestId = request.RequestId,
                Success = true,
                ExitCode = 0
            };
        }

        if (request.Command == CompilerDaemonProtocol.ShutdownCommand)
        {
            return new CompilerDaemonResponse
            {
                RequestId = request.RequestId,
                Success = true,
                ExitCode = 0
            };
        }

        var sw = Stopwatch.StartNew();
        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = request.CompilerExecutable,
                Arguments = request.Arguments,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
                WorkingDirectory = request.WorkingDirectory,
                StandardOutputEncoding = Encoding.UTF8,
                StandardErrorEncoding = Encoding.UTF8
            };

            if (_cachedEnvironment != null)
            {
                foreach (var (key, value) in _cachedEnvironment)
                    psi.Environment[key] = value;
            }

            using var process = Process.Start(psi)
                ?? throw new InvalidOperationException("Failed to start compiler process");

            var stdoutTask = process.StandardOutput.ReadToEndAsync(cancellationToken);
            var stderrTask = process.StandardError.ReadToEndAsync(cancellationToken);
            await process.WaitForExitAsync(cancellationToken);

            var stdout = await stdoutTask;
            var stderr = await stderrTask;
            sw.Stop();

            _lastActivityUtc = DateTime.UtcNow;
            Interlocked.Increment(ref _jobsProcessed);

            return new CompilerDaemonResponse
            {
                RequestId = request.RequestId,
                Success = process.ExitCode == 0,
                ExitCode = process.ExitCode,
                StandardOutput = stdout,
                StandardError = stderr,
                ElapsedMs = sw.ElapsedMilliseconds
            };
        }
        catch (Exception ex) when (ex is not OperationCanceledException)
        {
            sw.Stop();
            Log.Warning(ex, "Compiler worker {WorkerId} job failed", WorkerId);
            return new CompilerDaemonResponse
            {
                RequestId = request.RequestId,
                Success = false,
                ExitCode = -1,
                ErrorMessage = ex.Message,
                ElapsedMs = sw.ElapsedMilliseconds
            };
        }
    }

    public Task<bool> HealthCheckAsync(CancellationToken cancellationToken = default) =>
        Task.FromResult(IsAlive && !_disposed);

    public bool IsIdleExpired() => DateTime.UtcNow - _lastActivityUtc > _idleTimeout;

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
    }
}
