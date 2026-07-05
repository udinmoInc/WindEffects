using System.Buffers;
using System.IO.MemoryMappedFiles;

namespace IgniteBT.Core.Filesystem;

/// <summary>
/// Zero-copy file reads via memory-mapped I/O — eliminates repeated Read() syscalls for hashing and scanning.
/// </summary>
public static class MemoryMappedFileReader
{
    public static byte[] ReadAllBytes(string filePath)
    {
        if (!File.Exists(filePath)) return Array.Empty<byte>();

        var length = new FileInfo(filePath).Length;
        if (length == 0) return Array.Empty<byte>();

        using var mmf = MemoryMappedFile.CreateFromFile(filePath, FileMode.Open, null, 0, MemoryMappedFileAccess.Read);
        using var accessor = mmf.CreateViewAccessor(0, length, MemoryMappedFileAccess.Read);
        var buffer = new byte[length];
        accessor.ReadArray(0, buffer, 0, buffer.Length);
        return buffer;
    }

    public static void ReadIntoBuffer(string filePath, IBufferWriter<byte> writer)
    {
        if (!File.Exists(filePath)) return;

        var fileInfo = new FileInfo(filePath);
        var span = writer.GetSpan((int)fileInfo.Length);
        using var stream = File.OpenRead(filePath);
        int total = 0;
        while (total < fileInfo.Length)
        {
            var read = stream.Read(span[total..]);
            if (read == 0) break;
            total += read;
        }
        writer.Advance(total);
    }
}
