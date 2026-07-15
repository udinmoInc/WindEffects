using IgniteBT.Core.Hashing;
using IgniteBT.Core.Serialization;

namespace IgniteBT.Build.Orchestration;

/// <summary>
/// Fast no-op build detection — target &lt;100ms when nothing changed.
/// </summary>
public static class NoOpBuildDetector
{
    public static bool IsNoOpRequired(
        BuildManifest? previousManifest,
        string configHash,
        string toolchainHash,
        string graphHash,
        Dictionary<string, string> currentModuleHashes)
    {
        if (previousManifest == null) return false;
        if (previousManifest.ConfigHash != configHash) return false;
        if (previousManifest.ToolchainHash != toolchainHash) return false;
        if (previousManifest.GraphHash != graphHash) return false;

        foreach (var (module, hash) in currentModuleHashes)
        {
            if (!previousManifest.ModuleOutputHashes.TryGetValue(module, out var prev) || prev != hash)
                return false;
        }

        return true;
    }

    public static Dictionary<string, string> ComputeModuleHashes(
        IEnumerable<(string Name, string BuildCsPath, string ModuleDir, List<string> SourceFiles)> modules,
        FileHashCache fileHashes)
    {
        var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (var (name, buildCs, dir, sources) in modules)
        {
            var buildHash = fileHashes.GetHash(buildCs);
            var sourceHashes = sources.Select(fileHashes.GetHash).OrderBy(h => h);
            result[name] = FastHash.CombineHashes(buildHash, FastHash.CombineHashes(sourceHashes.ToArray()));
        }
        return result;
    }
}
