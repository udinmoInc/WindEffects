using System.Collections.Concurrent;
using IgniteBT.Build.Unity;
using Xunit;

namespace IgniteBT.Tests;

public class AdaptiveUnityFilterConcurrencyTests
{
    [Theory]
    [InlineData(2)]
    [InlineData(4)]
    [InlineData(8)]
    [InlineData(16)]
    [InlineData(32)]
    public void RecordBuild_IsThreadSafe_WithManyWorkers(int workerCount)
    {
        var tempDir = Path.Combine(Path.GetTempPath(), "ignitebt_unity_filter", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempDir);

        var files = Enumerable.Range(0, 256)
            .Select(i =>
            {
                var path = Path.Combine(tempDir, $"source_{i}.cpp");
                File.WriteAllText(path, $"int x{i}() {{ return {i}; }}");
                return path;
            })
            .ToArray();

        var filter = new AdaptiveUnityFilter(databaseDirectory: tempDir);
        var barrier = new Barrier(workerCount);
        var errors = new ConcurrentQueue<Exception>();

        Parallel.For(0, workerCount, workerId =>
        {
            try
            {
                barrier.SignalAndWait();
                for (var round = 0; round < 50; round++)
                {
                    foreach (var file in files)
                        filter.RecordBuild(file);
                }
            }
            catch (Exception ex)
            {
                errors.Enqueue(ex);
            }
        });

        Assert.Empty(errors);
        filter.SaveIfDirty();

        foreach (var file in files)
            Assert.True(File.Exists(file));

        try { Directory.Delete(tempDir, recursive: true); }
        catch { /* best effort */ }
    }

    [Fact]
    public async Task GetUnstableFiles_DoesNotThrowWhileRecording()
    {
        var tempDir = Path.Combine(Path.GetTempPath(), "ignitebt_unity_filter_rw", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempDir);

        var files = Enumerable.Range(0, 32)
            .Select(i =>
            {
                var path = Path.Combine(tempDir, $"rw_{i}.cpp");
                File.WriteAllText(path, $"int y{i}() {{ return {i}; }}");
                return path;
            })
            .ToArray();

        var filter = new AdaptiveUnityFilter(databaseDirectory: tempDir);
        using var cts = new CancellationTokenSource(TimeSpan.FromSeconds(3));
        var readErrors = new ConcurrentQueue<Exception>();
        var writeErrors = new ConcurrentQueue<Exception>();

        var readers = Task.Run(async () =>
        {
            while (!cts.IsCancellationRequested)
            {
                try
                {
                    _ = filter.GetUnstableFiles(files).ToList();
                    await Task.Yield();
                }
                catch (Exception ex)
                {
                    readErrors.Enqueue(ex);
                    break;
                }
            }
        });

        var writers = Task.Run(async () =>
        {
            while (!cts.IsCancellationRequested)
            {
                try
                {
                    foreach (var file in files)
                        filter.RecordBuild(file);
                    await Task.Yield();
                }
                catch (Exception ex)
                {
                    writeErrors.Enqueue(ex);
                    break;
                }
            }
        });

        await Task.WhenAll(readers, writers);
        Assert.Empty(readErrors);
        Assert.Empty(writeErrors);

        try { Directory.Delete(tempDir, recursive: true); }
        catch { /* best effort */ }
    }
}
