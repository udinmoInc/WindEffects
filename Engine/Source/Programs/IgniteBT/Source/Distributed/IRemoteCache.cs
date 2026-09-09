// ==============================================================================
// WindEffects — IgniteBT — IRemoteCache
// Source file for the IgniteBT module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
namespace IgniteBT.Distributed;

/// <summary>
/// Network-backed remote cache for object files and dependency graphs.
/// </summary>
public interface IRemoteCache
{
    Task<bool> TryGetAsync(string casKey, string localPath, CancellationToken cancellationToken = default);
    Task PutAsync(string casKey, string localPath, CancellationToken cancellationToken = default);
    Task<bool> ContainsAsync(string casKey, CancellationToken cancellationToken = default);
    RemoteCacheStats GetStats();
}

public sealed class RemoteCacheStats
{
    public long Hits { get; set; }
    public long Misses { get; set; }
    public long BytesTransferred { get; set; }
    public double HitRate => Hits + Misses > 0 ? (double)Hits / (Hits + Misses) : 0;
}
