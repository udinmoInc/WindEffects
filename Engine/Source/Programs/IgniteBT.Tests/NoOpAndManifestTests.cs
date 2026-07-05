using IgniteBT.Build.Orchestration;
using IgniteBT.Core.Hashing;
using IgniteBT.Core.Serialization;
using Xunit;

namespace IgniteBT.Tests;

public class NoOpAndManifestTests : IDisposable
{
    private readonly string _dir;

    public NoOpAndManifestTests()
    {
        _dir = Path.Combine(Path.GetTempPath(), "ignitebt_tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(_dir);
    }

    [Fact]
    public void NoOpDetector_ReturnsTrue_WhenManifestMatches()
    {
        var moduleHashes = new Dictionary<string, string> { ["Core"] = "abc123" };
        var manifest = new BuildManifest
        {
            ConfigHash = "cfg",
            ToolchainHash = "tool",
            GraphHash = "graph",
            ModuleOutputHashes = moduleHashes
        };

        var isNoOp = NoOpBuildDetector.IsNoOpRequired(manifest, "cfg", "tool", "graph", moduleHashes);
        Assert.True(isNoOp);
    }

    [Fact]
    public void NoOpDetector_ReturnsFalse_WhenModuleHashChanges()
    {
        var manifest = new BuildManifest
        {
            ConfigHash = "cfg",
            ToolchainHash = "tool",
            GraphHash = "graph",
            ModuleOutputHashes = new Dictionary<string, string> { ["Core"] = "old" }
        };

        var current = new Dictionary<string, string> { ["Core"] = "new" };
        Assert.False(NoOpBuildDetector.IsNoOpRequired(manifest, "cfg", "tool", "graph", current));
    }

    [Fact]
    public void BuildManifest_RoundTrips_OnDisk()
    {
        var path = Path.Combine(_dir, "build.manifest.bin");
        var manifest = new BuildManifest
        {
            ConfigHash = FastHash.HashString("Development"),
            ToolchainHash = FastHash.HashString("14.44"),
            GraphHash = FastHash.HashString("graph"),
            ModuleOutputHashes = new Dictionary<string, string> { ["Editor"] = "hash1" },
            BuiltUtc = DateTime.UtcNow
        };

        manifest.Save(path);
        Assert.True(File.Exists(path));
        Assert.True(BuildManifest.TryLoad(path, out var loaded));
        Assert.NotNull(loaded);
        Assert.Equal(manifest.ConfigHash, loaded!.ConfigHash);
        Assert.Equal(manifest.ModuleOutputHashes["Editor"], loaded.ModuleOutputHashes["Editor"]);
    }

    public void Dispose()
    {
        try { Directory.Delete(_dir, recursive: true); } catch { /* best effort */ }
    }
}
