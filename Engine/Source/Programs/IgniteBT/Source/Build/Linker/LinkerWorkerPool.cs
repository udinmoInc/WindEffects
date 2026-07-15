using IgniteBT.Build.Linker;
using IgniteBT.Build.Toolchain;

namespace IgniteBT.Build.Linker;

public sealed class LinkerWorkerPool : IDisposable
{
    private readonly ILinker _linker;
    private readonly SemaphoreSlim _slots;
    private volatile bool _disposed;

    public int MaxWorkers { get; }

    public LinkerWorkerPool(ILinker linker, int maxWorkers, string? vcVarsAllPath = null, string? persistDirectory = null)
    {
        _linker = linker;
        MaxWorkers = Math.Max(1, maxWorkers);
        _slots = new SemaphoreSlim(MaxWorkers, MaxWorkers);

        if (linker is MSVCLinker msvc)
        {
            var env = VcEnvironmentCache.LoadOrCapture(vcVarsAllPath, persistDirectory);
            if (env != null)
                msvc.SetEnvironmentVariables(env);
        }
    }

    public async Task<LinkResult> LinkAsync(LinkerOptions options, CancellationToken cancellationToken = default)
    {
        await _slots.WaitAsync(cancellationToken);
        try { return await _linker.LinkAsync(options); }
        finally { _slots.Release(); }
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        _slots.Dispose();
    }
}
