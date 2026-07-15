using IgniteBT.Core.Hashing;

namespace IgniteBT.Build.Orchestration;

/// <summary>
/// Hashes CLI build flags that affect compilation output.
/// </summary>
public static class BuildFlagsHasher
{
    public static string Compute(
        string? targetName,
        bool unityBuild,
        int unitySize,
        IEnumerable<string> unityDisabled,
        int jobs)
    {
        var disabled = string.Join(",", unityDisabled.OrderBy(s => s, StringComparer.OrdinalIgnoreCase));
        return FastHash.HashString(string.Join("|",
            targetName ?? string.Empty,
            unityBuild ? "1" : "0",
            unitySize.ToString(),
            disabled,
            jobs.ToString()));
    }
}
