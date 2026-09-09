// ==============================================================================
// WindEffects — IgniteBT — CpuAffinity
// Source file for the IgniteBT module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
