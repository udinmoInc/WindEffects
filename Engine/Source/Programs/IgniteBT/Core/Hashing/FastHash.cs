using System.Buffers.Binary;
using System.IO.Hashing;
using System.Text;
using IgniteBT.Core.Filesystem;

namespace IgniteBT.Core.Hashing;

/// <summary>
/// High-throughput hashing using xxHash3 (SIMD-accelerated on supported hardware).
/// Uses memory-mapped reads for large files to minimize syscalls.
/// </summary>
public static class FastHash
{
    private const long Seed = unchecked((long)0x9E3779B97F4A7C15UL);
    private const int MmapThreshold = 65536;

    public static string HashFile(string filePath)
    {
        if (!File.Exists(filePath)) return string.Empty;

        var length = new FileInfo(filePath).Length;
        if (length == 0) return ToHex(0);

        if (length >= MmapThreshold)
        {
            try
            {
                var data = MemoryMappedFileReader.ReadAllBytes(filePath);
                return HashBytes(data);
            }
            catch
            {
                // fall through to stream read
            }
        }

        using var stream = File.OpenRead(filePath);
        var hasher = new XxHash3(Seed);
        var buffer = new byte[65536];
        int read;
        while ((read = stream.Read(buffer, 0, buffer.Length)) > 0)
            hasher.Append(buffer.AsSpan(0, read));

        return ToHex(hasher.GetCurrentHashAsUInt64());
    }

    public static string HashBytes(ReadOnlySpan<byte> data) => ToHex(XxHash3.HashToUInt64(data, Seed));
    public static string HashString(string content) => HashBytes(Encoding.UTF8.GetBytes(content));

    public static string CombineHashes(params string[] hashes)
    {
        var hasher = new XxHash3(Seed);
        foreach (var hash in hashes.OrderBy(h => h, StringComparer.Ordinal))
            hasher.Append(Encoding.UTF8.GetBytes(hash));
        return ToHex(hasher.GetCurrentHashAsUInt64());
    }

    private static string ToHex(ulong value)
    {
        Span<byte> bytes = stackalloc byte[8];
        BinaryPrimitives.WriteUInt64LittleEndian(bytes, value);
        return Convert.ToHexString(bytes).ToLowerInvariant();
    }
}
