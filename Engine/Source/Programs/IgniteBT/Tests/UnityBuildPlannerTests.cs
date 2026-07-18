using IgniteBT.Build.Unity;
using Xunit;

namespace IgniteBT.Tests;

public class UnityBuildPlannerTests
{
    [Fact]
    public void Plan_IncludesSmallFilesAsSoloBlobs()
    {
        var root = Path.Combine(Path.GetTempPath(), "IgniteBTUnityPlanner_" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);

        try
        {
            var large = Path.Combine(root, "Large.cpp");
            var small = Path.Combine(root, "Small.cpp");
            File.WriteAllText(large, string.Join('\n', Enumerable.Repeat("// line", 80)));
            File.WriteAllText(small, string.Join('\n', Enumerable.Repeat("// line", 10)));

            var settings = new UnityBuildSettings
            {
                Enabled = true,
                MinFilesPerBlob = 1,
                MaxFilesPerBlob = 16,
                MaxLinesPerBlob = 8000,
                MinLinesForUnity = 50
            };

            var blobs = UnityBuildPlanner.Plan(new[] { large, small }, Array.Empty<string>(), settings);
            var planned = blobs.SelectMany(b => b.SourceFiles).ToHashSet(StringComparer.OrdinalIgnoreCase);

            Assert.Contains(large, planned);
            Assert.Contains(small, planned);
            Assert.Contains(blobs, b => b.SourceFiles.Count == 1 && b.SourceFiles[0] == small);
        }
        finally
        {
            Directory.Delete(root, recursive: true);
        }
    }
}
