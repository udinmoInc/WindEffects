using System.Diagnostics;
using IgniteBT.Build.Layout;
using IgniteBT.Core.Serialization;

namespace IgniteBT.Build.Orchestration;

public sealed class FastNoOpProbeResult
{
    public bool IsNoOp { get; init; }
    public long ElapsedMs { get; init; }
    public long ManifestLoadMs { get; init; }
    public long SnapshotLoadMs { get; init; }
    public long ValidateMs { get; init; }
    public string? SkipReason { get; init; }
}

/// <summary>
/// Sub-100ms no-op probe — runs before SDK, compiler, shaders, and orchestrator startup.
/// </summary>
public static class FastNoOpProbe
{
    public static FastNoOpProbeResult TryProbe(
        string engineDir,
        BuildLayout layout,
        string configName,
        string platform,
        string? targetName,
        string buildFlagsHash,
        int jobs,
        bool unityBuild,
        int unitySize,
        string unityDisabled)
    {
        var total = Stopwatch.StartNew();

        var manifestSw = Stopwatch.StartNew();
        var manifestPath = BuildManifest.GetPath(layout.ManifestDirectory);
        if (!File.Exists(manifestPath))
            return Fail(total, manifestSw.ElapsedMilliseconds, 0, 0, "no manifest");
        manifestSw.Stop();

        var snapshotSw = Stopwatch.StartNew();
        if (!WorkspaceSnapshot.TryLoad(layout.DatabaseDirectory, out var snapshot) || snapshot == null)
            return Fail(total, manifestSw.ElapsedMilliseconds, snapshotSw.ElapsedMilliseconds, 0, "no snapshot");
        snapshotSw.Stop();

        if (!snapshot.MatchesBuildRequest(configName, platform, targetName, buildFlagsHash, jobs, unityBuild, unitySize, unityDisabled))
            return Fail(total, manifestSw.ElapsedMilliseconds, snapshotSw.ElapsedMilliseconds, 0, "build flags changed");

        var validateSw = Stopwatch.StartNew();
        if (!snapshot.TryValidate(engineDir, out var validateReason))
        {
            validateSw.Stop();
            return Fail(total, manifestSw.ElapsedMilliseconds, snapshotSw.ElapsedMilliseconds, validateSw.ElapsedMilliseconds, validateReason);
        }
        validateSw.Stop();

        if (!string.IsNullOrEmpty(snapshot.ManifestConfigHash)
            && !string.IsNullOrEmpty(snapshot.ManifestToolchainHash))
        {
            // Snapshot is only written after manifest validation during the last build.
            total.Stop();
            return new FastNoOpProbeResult
            {
                IsNoOp = true,
                ElapsedMs = total.ElapsedMilliseconds,
                ManifestLoadMs = manifestSw.ElapsedMilliseconds,
                SnapshotLoadMs = snapshotSw.ElapsedMilliseconds,
                ValidateMs = validateSw.ElapsedMilliseconds
            };
        }

        manifestSw.Restart();
        if (!BuildManifest.TryLoad(manifestPath, out var manifest) || manifest == null)
            return Fail(total, manifestSw.ElapsedMilliseconds, snapshotSw.ElapsedMilliseconds, validateSw.ElapsedMilliseconds, "no manifest");
        manifestSw.Stop();

        if (manifest.ConfigHash != GraphSerializer.ComputeConfigHash(
                configName, platform, snapshot.CompilerVersion, snapshot.FeatureFlagsHash, snapshot.BuildFlagsHash)
            || manifest.ToolchainHash != IgniteBT.Core.Hashing.FastHash.HashString(snapshot.CompilerVersion))
        {
            return Fail(total, manifestSw.ElapsedMilliseconds, snapshotSw.ElapsedMilliseconds, validateSw.ElapsedMilliseconds, "manifest mismatch");
        }

        total.Stop();
        return new FastNoOpProbeResult
        {
            IsNoOp = true,
            ElapsedMs = total.ElapsedMilliseconds,
            ManifestLoadMs = manifestSw.ElapsedMilliseconds,
            SnapshotLoadMs = snapshotSw.ElapsedMilliseconds,
            ValidateMs = validateSw.ElapsedMilliseconds
        };
    }

    private static FastNoOpProbeResult Fail(
        Stopwatch total, long manifestMs, long snapshotMs, long validateMs, string? reason)
    {
        total.Stop();
        return new FastNoOpProbeResult
        {
            IsNoOp = false,
            ElapsedMs = total.ElapsedMilliseconds,
            ManifestLoadMs = manifestMs,
            SnapshotLoadMs = snapshotMs,
            ValidateMs = validateMs,
            SkipReason = reason
        };
    }
}
