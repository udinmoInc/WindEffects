using IgniteBT.Build.Orchestration;
using Xunit;

namespace IgniteBT.Tests;

public class FastNoOpProbeTests : IDisposable
{
    private readonly string _dir;
    private readonly string _projectRoot;
    private readonly string _engineRoot;

    public FastNoOpProbeTests()
    {
        _dir = Path.Combine(Path.GetTempPath(), "ignitebt_fastnoop", Guid.NewGuid().ToString("N"));
        _projectRoot = Path.Combine(_dir, "repo");
        _engineRoot = Path.Combine(_projectRoot, "Engine");
        var engineSource = Path.Combine(_engineRoot, "Source", "Test");
        Directory.CreateDirectory(engineSource);
        File.WriteAllText(Path.Combine(_projectRoot, "WindEffects.engine"), "schema=1");

        var buildCs = Path.Combine(engineSource, "Test.Build.cs");
        var cpp = Path.Combine(engineSource, "Test.cpp");
        File.WriteAllText(buildCs, "// build");
        File.WriteAllText(cpp, "int main(){}");

        var layout = IgniteBT.Build.Layout.BuildLayout.Resolve(_projectRoot, "Win64", IgniteBT.Build.Compiler.BuildConfiguration.Development);
        Directory.CreateDirectory(layout.ManifestDirectory);
        Directory.CreateDirectory(layout.DatabaseDirectory);

        var manifest = new IgniteBT.Core.Serialization.BuildManifest
        {
            ConfigHash = IgniteBT.Core.Serialization.GraphSerializer.ComputeConfigHash(
                "Development", "Win64", "14.0", "flags"),
            ToolchainHash = IgniteBT.Core.Hashing.FastHash.HashString("14.0"),
            GraphHash = "graph",
            ModuleOutputHashes = new Dictionary<string, string> { ["Test"] = "mod" }
        };
        manifest.Save(IgniteBT.Core.Serialization.BuildManifest.GetPath(layout.ManifestDirectory));

        WorkspaceSnapshot.Create(
            "Development",
            "Win64",
            null,
            BuildFlagsHasher.Compute(null, false, 0, [], Environment.ProcessorCount),
            "flags",
            manifest.ConfigHash,
            manifest.ToolchainHash,
            "14.0",
            "",
            Environment.ProcessorCount,
            false,
            0,
            "",
            [buildCs],
            [buildCs, cpp])
            .Save(layout.DatabaseDirectory);
    }

    [Fact]
    public void FastProbe_ReturnsNoOp_WhenSnapshotMatches()
    {
        var layout = IgniteBT.Build.Layout.BuildLayout.Resolve(_projectRoot, "Win64", IgniteBT.Build.Compiler.BuildConfiguration.Development);
        var flags = BuildFlagsHasher.Compute(null, false, 0, [], Environment.ProcessorCount);
        var result = FastNoOpProbe.TryProbe(_engineRoot, layout, "Development", "Win64", null, flags,
            Environment.ProcessorCount, false, 0, "");
        Assert.True(result.IsNoOp);
        Assert.True(result.ElapsedMs < 200);
    }

    [Fact]
    public void FastProbe_ReturnsFalse_WhenSourceChanged()
    {
        var cpp = Path.Combine(_engineRoot, "Source", "Test", "Test.cpp");
        File.AppendAllText(cpp, "\n// change");

        var layout = IgniteBT.Build.Layout.BuildLayout.Resolve(_projectRoot, "Win64", IgniteBT.Build.Compiler.BuildConfiguration.Development);
        var flags = BuildFlagsHasher.Compute(null, false, 0, [], Environment.ProcessorCount);
        var result = FastNoOpProbe.TryProbe(_engineRoot, layout, "Development", "Win64", null, flags,
            Environment.ProcessorCount, false, 0, "");
        Assert.False(result.IsNoOp);
    }

    public void Dispose()
    {
        try { Directory.Delete(_dir, recursive: true); } catch { /* best effort */ }
    }
}
