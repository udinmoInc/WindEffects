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
