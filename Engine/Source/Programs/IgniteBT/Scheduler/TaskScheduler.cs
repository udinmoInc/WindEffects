using System.Collections.Concurrent;
using Serilog;
using IgniteBT.BuildGraph;
using IgniteBT.ThreadPool;

namespace IgniteBT.Scheduler;

/// <summary>
/// Task status.
/// </summary>
public enum TaskStatus
{
    Pending,
    Running,
    Completed,
    Failed,
    Skipped
}

/// <summary>
/// A build task to be scheduled.
/// </summary>
public class BuildTask
{
    /// <summary>
    /// Task identifier.
    /// </summary>
    public string Id { get; set; } = string.Empty;

    /// <summary>
    /// Task name for display.
    /// </summary>
    public string Name { get; set; } = string.Empty;

    /// <summary>
    /// Task dependencies (other task IDs).
    /// </summary>
    public List<string> Dependencies { get; set; } = new();

    /// <summary>
    /// The work to execute.
    /// </summary>
    public Func<Task>? Work { get; set; }

    /// <summary>
    /// Current task status.
    /// </summary>
    public TaskStatus Status { get; set; }

    /// <summary>
    /// Task start time.
    /// </summary>
    public DateTime StartTime { get; set; }

    /// <summary>
    /// Task end time.
    /// </summary>
    public DateTime EndTime { get; set; }

    /// <summary>
    /// Task duration in milliseconds.
    /// </summary>
    public long DurationMs => (long)(EndTime - StartTime).TotalMilliseconds;

    /// <summary>
    /// Exception if task failed.
    /// </summary>
    public Exception? Exception { get; set; }

    /// <summary>
    /// Associated module node (if applicable).
    /// </summary>
    public BuildNode? ModuleNode { get; set; }
}

/// <summary>
/// Task scheduler for coordinating parallel build execution.
/// </summary>
public class TaskScheduler : IDisposable
{
    private readonly WorkStealingThreadPool _threadPool;
    private readonly ConcurrentDictionary<string, BuildTask> _tasks = new();
    private readonly ConcurrentDictionary<string, List<string>> _dependents = new();
    private readonly object _lock = new();
    private volatile bool _isDisposed;

    /// <summary>
    /// Total number of tasks.
    /// </summary>
    public int TotalTaskCount => _tasks.Count;

    /// <summary>
    /// Number of completed tasks.
    /// </summary>
    public int CompletedTaskCount => _tasks.Values.Count(t => t.Status == TaskStatus.Completed);

    /// <summary>
    /// Number of failed tasks.
    /// </summary>
    public int FailedTaskCount => _tasks.Values.Count(t => t.Status == TaskStatus.Failed);

    /// <summary>
    /// Progress (0.0 to 1.0).
    /// </summary>
    public double Progress => TotalTaskCount > 0 ? (double)CompletedTaskCount / TotalTaskCount : 0.0;

    /// <summary>
    /// Creates a new task scheduler.
    /// </summary>
    public TaskScheduler(int workerCount = -1)
    {
        _threadPool = new WorkStealingThreadPool(workerCount);
        _threadPool.Start();
        
        Log.Information("Task scheduler created with {WorkerCount} workers", _threadPool.WorkerCount);
    }

    /// <summary>
    /// Adds a task to the scheduler.
    /// </summary>
    public void AddTask(BuildTask task)
    {
        if (_isDisposed)
        {
            throw new ObjectDisposedException(nameof(TaskScheduler));
        }

        _tasks[task.Id] = task;

        // Build dependent map
        foreach (var dep in task.Dependencies)
        {
            if (!_dependents.ContainsKey(dep))
            {
                _dependents[dep] = new List<string>();
            }
            _dependents[dep].Add(task.Id);
        }

        Log.Debug("Added task {TaskId} with {DepCount} dependencies", task.Id, task.Dependencies.Count);
    }

    /// <summary>
    /// Adds tasks from a build graph.
    /// </summary>
    public void AddTasksFromGraph(List<BuildNode> buildOrder)
    {
        foreach (var node in buildOrder)
        {
            var task = new BuildTask
            {
                Id = node.Name,
                Name = $"Build {node.Name}",
                Dependencies = node.Dependencies.Select(d => d.Name).ToList(),
                ModuleNode = node
            };

            AddTask(task);
        }

        Log.Information("Added {Count} tasks from build graph", buildOrder.Count);
    }

    /// <summary>
    /// Executes all tasks respecting dependencies.
    /// </summary>
    public async Task<bool> ExecuteAsync()
    {
        Log.Information("Starting task execution with {Count} tasks", _tasks.Count);

        var startTime = DateTime.UtcNow;

        try
        {
            // Queue tasks that have no dependencies
            QueueReadyTasks();

            // Wait for all tasks to complete
            await WaitForCompletionAsync();

            var duration = (DateTime.UtcNow - startTime).TotalSeconds;
            Log.Information("Task execution completed in {Duration}s (Success: {Success}, Failed: {Failed})", 
                duration.ToString("F2"), CompletedTaskCount, FailedTaskCount);

            return FailedTaskCount == 0;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Task execution failed with exception");
            return false;
        }
    }

    /// <summary>
    /// Queues tasks whose dependencies are satisfied.
    /// </summary>
    private void QueueReadyTasks()
    {
        lock (_lock)
        {
            foreach (var task in _tasks.Values)
            {
                if (task.Status == TaskStatus.Pending && AreDependenciesSatisfied(task))
                {
                    QueueTask(task);
                }
            }
        }
    }

    /// <summary>
    /// Checks if a task's dependencies are satisfied.
    /// </summary>
    private bool AreDependenciesSatisfied(BuildTask task)
    {
        foreach (var depId in task.Dependencies)
        {
            if (_tasks.TryGetValue(depId, out var depTask))
            {
                if (depTask.Status != TaskStatus.Completed)
                {
                    return false;
                }
            }
            else
            {
                Log.Warning("Task {TaskId} depends on non-existent task {DepId}", task.Id, depId);
                return false;
            }
        }
        return true;
    }

    /// <summary>
    /// Queues a single task for execution.
    /// </summary>
    private void QueueTask(BuildTask task)
    {
        task.Status = TaskStatus.Running;
        task.StartTime = DateTime.UtcNow;

        _threadPool.QueueWork(async () => await ExecuteTask(task), task.Name);
    }

    /// <summary>
    /// Executes a task and handles completion.
    /// </summary>
    private async Task ExecuteTask(BuildTask task)
    {
        try
        {
            Log.Information("Executing task {TaskId}: {TaskName}", task.Id, task.Name);

            if (task.Work != null)
            {
                await task.Work();
            }

            task.Status = TaskStatus.Completed;
            task.EndTime = DateTime.UtcNow;

            Log.Information("Task {TaskId} completed in {Duration}ms", task.Id, task.DurationMs);

            // Queue dependent tasks
            QueueDependentTasks(task.Id);
        }
        catch (Exception ex)
        {
            task.Status = TaskStatus.Failed;
            task.Exception = ex;
            task.EndTime = DateTime.UtcNow;

            Log.Error(ex, "Task {TaskId} failed after {Duration}ms", task.Id, task.DurationMs);

            // Mark dependents as failed
            MarkDependentsFailed(task.Id);
        }
    }

    /// <summary>
    /// Queues tasks that depend on a completed task.
    /// </summary>
    private void QueueDependentTasks(string completedTaskId)
    {
        if (!_dependents.TryGetValue(completedTaskId, out var dependentIds))
        {
            return;
        }

        lock (_lock)
        {
            foreach (var depId in dependentIds)
            {
                if (_tasks.TryGetValue(depId, out var dependentTask))
                {
                    if (dependentTask.Status == TaskStatus.Pending && AreDependenciesSatisfied(dependentTask))
                    {
                        QueueTask(dependentTask);
                    }
                }
            }
        }
    }

    /// <summary>
    /// Marks all dependents of a failed task as failed.
    /// </summary>
    private void MarkDependentsFailed(string failedTaskId)
    {
        if (!_dependents.TryGetValue(failedTaskId, out var dependentIds))
        {
            return;
        }

        foreach (var depId in dependentIds)
        {
            if (_tasks.TryGetValue(depId, out var dependentTask))
            {
                if (dependentTask.Status == TaskStatus.Pending)
                {
                    dependentTask.Status = TaskStatus.Failed;
                    dependentTask.Exception = new InvalidOperationException($"Dependency {failedTaskId} failed");
                    Log.Warning("Task {TaskId} marked as failed due to dependency {DepId} failure", depId, failedTaskId);

                    // Recursively mark dependents
                    MarkDependentsFailed(depId);
                }
            }
        }
    }

    /// <summary>
    /// Waits for all tasks to complete.
    /// </summary>
    private async Task WaitForCompletionAsync()
    {
        while (true)
        {
            var pendingCount = _tasks.Values.Count(t => t.Status == TaskStatus.Pending);
            var runningCount = _tasks.Values.Count(t => t.Status == TaskStatus.Running);

            if (pendingCount == 0 && runningCount == 0)
            {
                break;
            }

            await Task.Delay(100);
        }
    }

    /// <summary>
    /// Gets scheduler statistics.
    /// </summary>
    public SchedulerStats GetStats()
    {
        var tasks = _tasks.Values.ToList();
        
        return new SchedulerStats
        {
            TotalTasks = tasks.Count,
            PendingTasks = tasks.Count(t => t.Status == TaskStatus.Pending),
            RunningTasks = tasks.Count(t => t.Status == TaskStatus.Running),
            CompletedTasks = tasks.Count(t => t.Status == TaskStatus.Completed),
            FailedTasks = tasks.Count(t => t.Status == TaskStatus.Failed),
            Progress = Progress,
            ThreadPoolStats = _threadPool.GetStats(),
            TotalDurationMs = tasks.Where(t => t.Status == TaskStatus.Completed).Sum(t => t.DurationMs)
        };
    }

    /// <summary>
    /// Gets all tasks.
    /// </summary>
    public IReadOnlyCollection<BuildTask> GetAllTasks()
    {
        return _tasks.Values.ToList().AsReadOnly();
    }

    /// <summary>
    /// Gets a task by ID.
    /// </summary>
    public BuildTask? GetTask(string id)
    {
        return _tasks.TryGetValue(id, out var task) ? task : null;
    }

    /// <summary>
    /// Clears all tasks.
    /// </summary>
    public void Clear()
    {
        _tasks.Clear();
        _dependents.Clear();
        Log.Information("Task scheduler cleared");
    }

    public void Dispose()
    {
        if (_isDisposed)
        {
            return;
        }

        _isDisposed = true;
        _threadPool.Dispose();
        Log.Information("Task scheduler disposed");
    }
}

/// <summary>
/// Scheduler statistics.
/// </summary>
public class SchedulerStats
{
    /// <summary>
    /// Total number of tasks.
    /// </summary>
    public int TotalTasks { get; set; }

    /// <summary>
    /// Number of pending tasks.
    /// </summary>
    public int PendingTasks { get; set; }

    /// <summary>
    /// Number of running tasks.
    /// </summary>
    public int RunningTasks { get; set; }

    /// <summary>
    /// Number of completed tasks.
    /// </summary>
    public int CompletedTasks { get; set; }

    /// <summary>
    /// Number of failed tasks.
    /// </summary>
    public int FailedTasks { get; set; }

    /// <summary>
    /// Progress (0.0 to 1.0).
    /// </summary>
    public double Progress { get; set; }

    /// <summary>
    /// Thread pool statistics.
    /// </summary>
    public ThreadPoolStats ThreadPoolStats { get; set; } = new();

    /// <summary>
    /// Total duration of all completed tasks in milliseconds.
    /// </summary>
    public long TotalDurationMs { get; set; }
}
