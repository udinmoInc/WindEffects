// ==============================================================================
// WindEffects — IgniteBT — IRemoteWorker
// Source file for the IgniteBT module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
namespace IgniteBT.Distributed;

/// <summary>
/// Represents a remote build worker node in a distributed build farm.
/// </summary>
public interface IRemoteWorker
{
    string WorkerId { get; }
    string Address { get; }
    int AvailableSlots { get; }
    RemoteWorkerCapabilities Capabilities { get; }
    Task<bool> PingAsync(CancellationToken cancellationToken = default);
}

public sealed class RemoteWorkerCapabilities
{
    public bool SupportsMSVC { get; set; }
    public bool SupportsClang { get; set; }
    public bool SupportsGCC { get; set; }
    public int LogicalCores { get; set; }
    public long MemoryBytes { get; set; }
    public string Platform { get; set; } = string.Empty;
}
