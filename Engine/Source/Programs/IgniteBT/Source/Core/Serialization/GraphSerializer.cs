using System.Text.Json;
using IgniteBT.Build.Graph;
using IgniteBT.Core.Hashing;
using Serilog;

namespace IgniteBT.Core.Serialization;

public sealed class SerializedGraphNode
{
    public string Name { get; set; } = string.Empty;
    public string ModuleHash { get; set; } = string.Empty;
    public List<string> Dependencies { get; set; } = new();
}

public sealed class SerializedGraph
{
    public string Version { get; set; } = "2.0";
    public string ConfigHash { get; set; } = string.Empty;
    public List<SerializedGraphNode> Nodes { get; set; } = new();
    public DateTime SavedUtc { get; set; }
}

/// <summary>
/// Persistent dependency graph serialization — avoids rebuilding graph from scratch.
/// </summary>
public static class GraphSerializer
{
    public static void Save(string path, DependencyGraph graph, string configHash)
    {
        var data = new SerializedGraph
        {
            ConfigHash = configHash,
            SavedUtc = DateTime.UtcNow,
            Nodes = graph.Nodes.Select(n => new SerializedGraphNode
            {
                Name = n.Name,
                ModuleHash = n.ContentHash,
                Dependencies = n.Dependencies.Select(d => d.Name).ToList()
            }).ToList()
        };

        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        File.WriteAllText(path, JsonSerializer.Serialize(data, new JsonSerializerOptions { WriteIndented = false }));
    }

    public static bool TryLoad(string path, string configHash, out SerializedGraph? graph)
    {
        graph = null;
        if (!File.Exists(path)) return false;

        try
        {
            graph = JsonSerializer.Deserialize<SerializedGraph>(File.ReadAllText(path));
            return graph != null && graph.ConfigHash == configHash;
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to load serialized graph");
            return false;
        }
    }

    public static string ComputeConfigHash(
        string config,
        string platform,
        string compilerVersion,
        string featureFlagsHash,
        string buildFlagsHash = "")
        => FastHash.CombineHashes(config, platform, compilerVersion, featureFlagsHash, buildFlagsHash ?? "");
}
