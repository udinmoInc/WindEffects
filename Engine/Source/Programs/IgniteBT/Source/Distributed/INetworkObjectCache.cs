// ==============================================================================
// WindEffects — IgniteBT — INetworkObjectCache
// Source file for the IgniteBT module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
namespace IgniteBT.Distributed;

/// <summary>
/// Network object cache with compression for distributed build artifact sharing.
/// </summary>
public interface INetworkObjectCache
{
    Task<bool> TryFetchAsync(string hash, Stream destination, CancellationToken cancellationToken = default);
    Task StoreAsync(string hash, Stream source, CancellationToken cancellationToken = default);
    Task PrefetchAsync(IReadOnlyList<string> hashes, CancellationToken cancellationToken = default);
    NetworkCacheMetrics GetMetrics();
}

public sealed class NetworkCacheMetrics
{
    public long LocalHits { get; set; }
    public long NetworkHits { get; set; }
    public long Misses { get; set; }
    public long BytesCompressed { get; set; }
    public long BytesTransferred { get; set; }
}
