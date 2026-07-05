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
