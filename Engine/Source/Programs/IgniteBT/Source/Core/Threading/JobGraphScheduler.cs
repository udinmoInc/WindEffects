using System.Collections.Concurrent;
using Serilog;
using IgniteBT.Build.Scheduler;

namespace IgniteBT.Core.Threading;

/// <summary>
/// Dependency-aware job scheduler with work-stealing execution and smart prioritization.
/// </summary>
public sealed class JobGraphScheduler : IDisposable
{
    private readonly JobGraph _graph;
    private readonly WorkStealingThreadPool _pool;
    private readonly SmartJobPrioritizer _prioritizer;
    private readonly CountdownEvent _completion = new(1);
    private readonly object _scheduleLock = new();
    private readonly HashSet<string> _queued = new(StringComparer.Ordinal);
    private volatile bool _failed;
    private volatile bool _disposed;

    public JobGraphScheduler(JobGraph graph, int workerCount = -1, SmartJobPrioritizer? prioritizer = null)
    {
        _graph = graph;
        _pool = new WorkStealingThreadPool(workerCount);
        _prioritizer = prioritizer ?? new SmartJobPrioritizer();
        _pool.Start();
    }

    public SmartJobPrioritizer Prioritizer => _prioritizer;

    public async Task<bool> ExecuteAsync(CancellationToken cancellationToken = default)
    {
        ScheduleReadyJobs();

        await Task.Run(() => _completion.Wait(cancellationToken), cancellationToken);
        return !_failed && _graph.FailedCount == 0;
    }

    private void ScheduleReadyJobs()
    {
        lock (_scheduleLock)
        {
            var ready = _prioritizer.Prioritize(_graph.GetReadyNodes()).ToList();
            foreach (var node in ready)
                QueueJob(node);
        }
    }

    private void QueueJob(JobNode node)
    {
        lock (_scheduleLock)
        {
            if (node.Status != JobStatus.Pending || !_queued.Add(node.Id))
                return;
        }

        _graph.MarkRunning(node);
        node.StartTicks = DateTime.UtcNow.Ticks;

        _pool.QueueWorkToLocal(async () =>
        {
            try
            {
                if (node.Work != null)
                    await node.Work(CancellationToken.None);

                _graph.MarkCompleted(node);
                OnJobFinished();
            }
            catch (Exception ex)
            {
                _failed = true;
                _graph.MarkFailed(node, ex);
                Log.Error(ex, "Job {JobId} failed", node.Id);
                OnJobFinished();
            }
        }, node.Name);
    }

    private void OnJobFinished()
    {
        ScheduleReadyJobs();
        if (_graph.IsComplete)
        {
            try { _completion.Signal(); }
            catch (InvalidOperationException) { }
        }
    }

    public long IdleMs => _pool.IdleSignals;

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        _pool.Dispose();
        _completion.Dispose();
    }
}
