using System.Buffers.Binary;
using System.Text;
using System.Text.Json;
using IgniteBT.Core.Hashing;

namespace IgniteBT.Core.Serialization;

/// <summary>
/// Binary build manifest for instant no-op detection and cross-build reuse.
/// </summary>
public sealed class BuildManifest
{
    public string Version { get; set; } = "2.0";
    public string ConfigHash { get; set; } = string.Empty;
    public string ToolchainHash { get; set; } = string.Empty;
    public string GraphHash { get; set; } = string.Empty;
    public Dictionary<string, string> ModuleOutputHashes { get; set; } = new();
    public Dictionary<string, string> ObjectHashes { get; set; } = new();
    public DateTime BuiltUtc { get; set; }

    public static string GetPath(string manifestDirectory) => Path.Combine(manifestDirectory, "build.manifest.bin");

    public void Save(string path)
    {
        var json = JsonSerializer.SerializeToUtf8Bytes(this);
        using var stream = File.Create(path);
        stream.Write("IGBTMF02"u8);
        var lenBytes = new byte[4];
        BinaryPrimitives.WriteInt32LittleEndian(lenBytes, json.Length);
        stream.Write(lenBytes);
        stream.Write(json);
    }

    public static bool TryLoad(string path, out BuildManifest? manifest)
    {
        manifest = null;
        if (!File.Exists(path)) return false;

        using var stream = File.OpenRead(path);
        var magic = new byte[8];
        if (stream.Read(magic, 0, 8) != 8) return false;

        var lenBytes = new byte[4];
        stream.Read(lenBytes, 0, 4);
        var len = BinaryPrimitives.ReadInt32LittleEndian(lenBytes);
        var json = new byte[len];
        stream.Read(json, 0, len);
        manifest = JsonSerializer.Deserialize<BuildManifest>(json);
        return manifest != null;
    }

    public string ComputeFingerprint() =>
        FastHash.CombineHashes(ConfigHash, ToolchainHash, GraphHash,
            FastHash.CombineHashes(ModuleOutputHashes.Values.OrderBy(v => v).ToArray()));
}
