using IgniteBT.Core.Threading;
using Xunit;

namespace IgniteBT.Tests;

public class JobGraphTests
{
    [Fact]
    public void MarkCompleted_IsIdempotent()
    {
        var graph = new JobGraph();
        var node = new JobNode { Id = "a", Name = "A" };
        graph.Add(node);
        graph.MarkRunning(node);
        graph.MarkCompleted(node);
        graph.MarkCompleted(node);
        Assert.Equal(JobStatus.Completed, node.Status);
    }

    [Fact]
    public void MarkFailed_DoesNotDoubleDecrementOnCascade()
    {
        var graph = new JobGraph();
        var a = new JobNode { Id = "a", Name = "A" };
        var b = new JobNode { Id = "b", Name = "B", Dependencies = { "a" } };
        graph.Add(a);
        graph.Add(b);
        graph.MarkRunning(a);
        graph.MarkFailed(a, new InvalidOperationException("fail"));
        Assert.Equal(JobStatus.Failed, b.Status);
        graph.MarkCompleted(a);
        Assert.Equal(JobStatus.Failed, a.Status);
    }

    [Fact]
    public async Task Scheduler_RunsAllJobs_WithoutException()
    {
        var graph = new JobGraph();
        var count = 0;
        for (var i = 0; i < 64; i++)
        {
            var id = $"job{i}";
            graph.Add(new JobNode
            {
                Id = id,
                Name = id,
                Work = _ =>
                {
                    Interlocked.Increment(ref count);
                    return Task.CompletedTask;
                }
            });
        }

        using var scheduler = new JobGraphScheduler(graph, workerCount: 16);
        var success = await scheduler.ExecuteAsync();
        Assert.True(success);
        Assert.Equal(64, count);
        Assert.Equal(0, graph.FailedCount);
    }
}
