using System.Runtime.InteropServices;

namespace IgniteBT.Core.Threading;

/// <summary>
/// CPU affinity helpers for worker thread pinning.
/// </summary>
public static class CpuAffinity
{
    public static void TryPinThreadToCore(int coreIndex)
    {
        if (!OperatingSystem.IsWindows()) return;
        try
        {
            var mask = 1UL << (coreIndex % Environment.ProcessorCount);
            SetThreadAffinityMask(GetCurrentThread(), new IntPtr((long)mask));
        }
        catch { /* best effort */ }
    }

    [DllImport("kernel32.dll")]
    private static extern IntPtr GetCurrentThread();

    [DllImport("kernel32.dll")]
    private static extern IntPtr SetThreadAffinityMask(IntPtr hThread, IntPtr dwThreadAffinityMask);
}
