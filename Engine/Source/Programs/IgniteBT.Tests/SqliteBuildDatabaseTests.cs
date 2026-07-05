using IgniteBT.Core.Database;
using Xunit;

namespace IgniteBT.Tests;

public sealed class SqliteBuildDatabaseTests : IDisposable
{
    private readonly string _dir;
    private readonly SqliteBuildDatabase _db;

    public SqliteBuildDatabaseTests()
    {
        _dir = Path.Combine(Path.GetTempPath(), "ignitebt_tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(_dir);
        _db = new SqliteBuildDatabase(_dir);
        _db.SetIncludeGraph(@"C:\src\main.cpp", ["a.h", "b.h", "c.h"]);
    }

    [Fact]
    public void ConcurrentIncludeGraphReads_DoNotThrow()
    {
        var errors = 0;
        Parallel.For(0, 256, i =>
        {
            try
            {
                for (var n = 0; n < 50; n++)
                {
                    if (_db.TryGetIncludeGraph(@"C:\src\main.cpp", out var headers))
                    {
                        Assert.Equal(3, headers.Count);
                    }
                    _db.IncrementCacheStat("object", true);
                    _ = _db.GetAverageCompileTime(@"C:\src\main.cpp");
                }
            }
            catch
            {
                Interlocked.Increment(ref errors);
            }
        });

        Assert.Equal(0, errors);
    }

    [Fact]
    public void ConcurrentIncludeGraphWrites_AreSerialized()
    {
        Parallel.For(0, 32, i =>
            _db.SetIncludeGraph($@"C:\src\file{i}.cpp", [$"h{i}.h"]));

        for (var i = 0; i < 32; i++)
        {
            Assert.True(_db.TryGetIncludeGraph($@"C:\src\file{i}.cpp", out var headers));
            Assert.Single(headers);
        }
    }

    public void Dispose()
    {
        _db.Dispose();
        try { Directory.Delete(_dir, recursive: true); } catch { /* best effort */ }
    }
}
