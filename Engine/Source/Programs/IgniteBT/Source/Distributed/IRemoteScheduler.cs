namespace IgniteBT.Distributed;

/// <summary>
/// Schedules build jobs across local and remote workers.
/// </summary>
public interface IRemoteScheduler
{
    Task ScheduleAsync(IReadOnlyList<RemoteJobDescriptor> jobs, CancellationToken cancellationToken = default);
    IReadOnlyList<IRemoteWorker> GetAvailableWorkers();
    RemoteSchedulerStats GetStats();
}

public sealed class RemoteJobDescriptor
{
    public string JobId { get; set; } = string.Empty;
    public string ModuleName { get; set; } = string.Empty;
    public string SourceFile { get; set; } = string.Empty;
    public int Priority { get; set; }
    public List<string> Dependencies { get; set; } = new();
}

public sealed class RemoteSchedulerStats
{
    public int LocalJobs { get; set; }
    public int RemoteJobs { get; set; }
    public int PendingJobs { get; set; }
    public long TotalRemoteCompileMs { get; set; }
}
