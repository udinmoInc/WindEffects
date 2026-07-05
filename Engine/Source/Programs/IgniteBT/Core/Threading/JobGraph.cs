namespace IgniteBT.Core.Threading;

public enum JobStatus { Pending, Ready, Running, Completed, Failed, Cancelled }

/// <summary>
/// A node in the dependency-aware job graph (modules, TUs, links, scans).
/// </summary>
public sealed class JobNode
{
    public string Id { get; init; } = string.Empty;
    public string Name { get; init; } = string.Empty;
    public List<string> Dependencies { get; } = new();
    public Func<CancellationToken, Task>? Work { get; set; }
    public JobStatus Status { get; set; } = JobStatus.Pending;
    public Exception? Exception { get; set; }
    public long StartTicks { get; set; }
    public long EndTicks { get; set; }
    public long DurationMs => (EndTicks - StartTicks) / TimeSpan.TicksPerMillisecond;
    public object? UserData { get; set; }
}

/// <summary>
/// Directed acyclic job graph with topological scheduling.
/// </summary>
public sealed class JobGraph
{
    private readonly Dictionary<string, JobNode> _nodes = new();
    private readonly Dictionary<string, List<string>> _dependents = new();
    private int _pendingCount;

    public int NodeCount => _nodes.Count;
    public int PendingCount => Volatile.Read(ref _pendingCount);

    public JobNode Add(JobNode node)
    {
        _nodes[node.Id] = node;
        Interlocked.Increment(ref _pendingCount);
        foreach (var dep in node.Dependencies)
        {
            if (!_dependents.ContainsKey(dep)) _dependents[dep] = new List<string>();
            _dependents[dep].Add(node.Id);
        }
        return node;
    }

    public IEnumerable<JobNode> GetReadyNodes()
    {
        foreach (var node in _nodes.Values)
        {
            if (node.Status != JobStatus.Pending) continue;
            var ready = true;
            foreach (var depId in node.Dependencies)
            {
                if (!_nodes.TryGetValue(depId, out var dep) || dep.Status != JobStatus.Completed)
                {
                    ready = false;
                    break;
                }
            }
            if (ready) yield return node;
        }
    }

    public void MarkRunning(JobNode node)
    {
        if (node.Status != JobStatus.Pending)
            return;
        node.Status = JobStatus.Running;
    }

    public void MarkCompleted(JobNode node)
    {
        if (node.Status != JobStatus.Running)
            return;
        node.Status = JobStatus.Completed;
        node.EndTicks = DateTime.UtcNow.Ticks;
        Interlocked.Decrement(ref _pendingCount);
    }

    public void MarkFailed(JobNode node, Exception ex)
    {
        if (node.Status is JobStatus.Completed or JobStatus.Failed or JobStatus.Cancelled)
            return;
        node.Status = JobStatus.Failed;
        node.Exception = ex;
        node.EndTicks = DateTime.UtcNow.Ticks;
        Interlocked.Decrement(ref _pendingCount);
        CascadeFailure(node.Id);
    }

    private void CascadeFailure(string failedId)
    {
        if (!_dependents.TryGetValue(failedId, out var deps)) return;
        foreach (var depId in deps)
        {
            if (!_nodes.TryGetValue(depId, out var node)) continue;
            if (node.Status is JobStatus.Completed or JobStatus.Failed or JobStatus.Cancelled)
                continue;
            node.Status = JobStatus.Failed;
            node.Exception = new InvalidOperationException($"Dependency {failedId} failed");
            node.EndTicks = DateTime.UtcNow.Ticks;
            Interlocked.Decrement(ref _pendingCount);
            CascadeFailure(depId);
        }
    }

    public bool IsComplete => _nodes.Values.All(n =>
        n.Status is JobStatus.Completed or JobStatus.Failed or JobStatus.Cancelled);

    public int FailedCount => _nodes.Values.Count(n => n.Status == JobStatus.Failed);
    public IReadOnlyCollection<JobNode> Nodes => _nodes.Values;
}
