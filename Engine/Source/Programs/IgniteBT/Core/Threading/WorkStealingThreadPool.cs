using System.Collections.Concurrent;
using Serilog;

namespace IgniteBT.Core.Threading;

/// <summary>
/// A work item to be executed by the thread pool.
/// </summary>
public class WorkItem
{
    /// <summary>
    /// The work to execute.
    /// </summary>
    public Func<Task>? AsyncWork { get; set; }

    /// <summary>
    /// Synchronous work to execute.
    /// </summary>
    public Action? SyncWork { get; set; }

    /// <summary>
    /// Work item name for debugging.
    /// </summary>
    public string Name { get; set; } = string.Empty;

    /// <summary>
    /// Whether this work item has been completed.
    /// </summary>
    public bool IsCompleted { get; set; }

    /// <summary>
    /// Exception that occurred during execution, if any.
    /// </summary>
    public Exception? Exception { get; set; }
}

/// <summary>
/// Local queue for a worker thread (supports work stealing).
/// </summary>
public class WorkStealingQueue
{
    private readonly ConcurrentQueue<WorkItem> _queue = new();
    private readonly object _lock = new();

    /// <summary>
    /// Number of items in the queue.
    /// </summary>
    public int Count => _queue.Count;

    /// <summary>
    /// Pushes a work item to the local queue.
    /// </summary>
    public void Push(WorkItem item)
    {
        lock (_lock)
        {
            _queue.Enqueue(item);
        }
    }

    /// <summary>
    /// Pops a work item from the local queue.
    /// </summary>
    public bool TryPop(out WorkItem? item)
    {
        lock (_lock)
        {
            return _queue.TryDequeue(out item);
        }
    }

    /// <summary>
    /// Steals a work item from the queue (called by other threads).
    /// </summary>
    public bool TrySteal(out WorkItem? item)
    {
        lock (_lock)
        {
            return _queue.TryDequeue(out item);
        }
    }
}

/// <summary>
/// Worker thread in the thread pool.
/// </summary>
public class WorkerThread
{
    private readonly WorkStealingThreadPool _pool;
    private readonly WorkStealingQueue _localQueue;
    private readonly Thread _thread;
    private readonly CancellationTokenSource _cts;
    private volatile bool _isRunning;

    public int Id { get; }
    public WorkStealingQueue LocalQueue => _localQueue;
    public bool IsRunning => _isRunning;

    public WorkerThread(WorkStealingThreadPool pool, int id)
    {
        _pool = pool;
        Id = id;
        _localQueue = new WorkStealingQueue();
        _cts = new CancellationTokenSource();
        _thread = new Thread(Run) { Name = $"Worker-{id}", IsBackground = true };
    }

    /// <summary>
    /// Starts the worker thread.
    /// </summary>
    public void Start()
    {
        _isRunning = true;
        _thread.Start();
    }

    /// <summary>
    /// Stops the worker thread.
    /// </summary>
    public void Stop()
    {
        _isRunning = false;
        _cts.Cancel();
        _thread.Join(5000); // Wait up to 5 seconds
    }

    /// <summary>
    /// Main worker loop.
    /// </summary>
    private async void Run()
    {
        Log.Information("Worker thread {WorkerId} started", Id);

        try
        {
            while (_isRunning && !_cts.Token.IsCancellationRequested)
            {
                // Try to get work from local queue first
                if (_localQueue.TryPop(out var item) && item != null)
                {
                    await ExecuteWorkItem(item);
                    continue;
                }

                // Try to steal work from other workers
                if (_pool.TryStealWork(this, out item) && item != null)
                {
                    await ExecuteWorkItem(item);
                    continue;
                }

                // No work available, wait a bit
                await Task.Delay(10, _cts.Token);
            }
        }
        catch (OperationCanceledException)
        {
            // Expected during shutdown
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Worker thread {WorkerId} crashed", Id);
        }

        Log.Information("Worker thread {WorkerId} stopped", Id);
    }

    /// <summary>
    /// Executes a work item.
    /// </summary>
    private async Task ExecuteWorkItem(WorkItem item)
    {
        try
        {
            Log.Debug("Worker {WorkerId} executing: {WorkItem}", Id, item.Name);

            if (item.AsyncWork != null)
            {
                await item.AsyncWork();
            }
            else if (item.SyncWork != null)
            {
                item.SyncWork();
            }

            item.IsCompleted = true;
            Log.Debug("Worker {WorkerId} completed: {WorkItem}", Id, item.Name);
        }
        catch (Exception ex)
        {
            item.Exception = ex;
            item.IsCompleted = true;
            Log.Error(ex, "Worker {WorkerId} failed to execute {WorkItem}", Id, item.Name);
        }
    }
}

/// <summary>
/// Work-stealing thread pool for parallel task execution.
/// </summary>
public class WorkStealingThreadPool : IDisposable
{
    private readonly List<WorkerThread> _workers;
    private readonly ConcurrentQueue<WorkItem> _globalQueue;
    private readonly Random _random;
    private volatile bool _isDisposed;

    /// <summary>
    /// Number of worker threads.
    /// </summary>
    public int WorkerCount => _workers.Count;

    /// <summary>
    /// Creates a new work-stealing thread pool.
    /// </summary>
    public WorkStealingThreadPool(int workerCount = -1)
    {
        _workers = new List<WorkerThread>();
        _globalQueue = new ConcurrentQueue<WorkItem>();
        _random = new Random();

        // Default to number of logical processors
        if (workerCount <= 0)
        {
            workerCount = Environment.ProcessorCount;
        }

        // Create worker threads
        for (int i = 0; i < workerCount; i++)
        {
            var worker = new WorkerThread(this, i);
            _workers.Add(worker);
        }

        Log.Information("Created work-stealing thread pool with {WorkerCount} workers", workerCount);
    }

    /// <summary>
    /// Starts the thread pool.
    /// </summary>
    public void Start()
    {
        foreach (var worker in _workers)
        {
            worker.Start();
        }

        Log.Information("Work-stealing thread pool started");
    }

    /// <summary>
    /// Stops the thread pool.
    /// </summary>
    public void Stop()
    {
        foreach (var worker in _workers)
        {
            worker.Stop();
        }

        Log.Information("Work-stealing thread pool stopped");
    }

    /// <summary>
    /// Queues work to be executed.
    /// </summary>
    public void QueueWork(WorkItem item)
    {
        if (_isDisposed)
        {
            throw new ObjectDisposedException(nameof(WorkStealingThreadPool));
        }

        // Add to global queue
        _globalQueue.Enqueue(item);
    }

    /// <summary>
    /// Queues async work to be executed.
    /// </summary>
    public void QueueWork(Func<Task> work, string name = "Unnamed")
    {
        QueueWork(new WorkItem
        {
            AsyncWork = work,
            Name = name
        });
    }

    /// <summary>
    /// Queues synchronous work to be executed.
    /// </summary>
    public void QueueWork(Action work, string name = "Unnamed")
    {
        QueueWork(new WorkItem
        {
            SyncWork = work,
            Name = name
        });
    }

    /// <summary>
    /// Tries to steal work from another worker's queue.
    /// </summary>
    internal bool TryStealWork(WorkerThread requester, out WorkItem? item)
    {
        item = null;

        // First check global queue
        if (_globalQueue.TryDequeue(out item))
        {
            return true;
        }

        // Try to steal from other workers
        var otherWorkers = _workers.Where(w => w.Id != requester.Id).ToList();
        if (otherWorkers.Count == 0)
        {
            return false;
        }

        // Randomly select a worker to steal from
        var targetIndex = _random.Next(otherWorkers.Count);
        var targetWorker = otherWorkers[targetIndex];

        // Try to steal from the target worker's queue
        if (targetWorker.LocalQueue.TrySteal(out item))
        {
            Log.Debug("Worker {RequesterId} stole work from worker {TargetId}", requester.Id, targetWorker.Id);
            return true;
        }

        return false;
    }

    /// <summary>
    /// Gets statistics about the thread pool.
    /// </summary>
    public ThreadPoolStats GetStats()
    {
        var localQueueSizes = _workers.Select(w => w.LocalQueue.Count).ToList();
        
        return new ThreadPoolStats
        {
            WorkerCount = _workers.Count,
            RunningWorkers = _workers.Count(w => w.IsRunning),
            GlobalQueueSize = _globalQueue.Count,
            LocalQueueSizes = localQueueSizes,
            TotalLocalQueueSize = localQueueSizes.Sum()
        };
    }

    /// <summary>
    /// Waits for all queued work to complete.
    /// </summary>
    public async Task WaitForCompletionAsync()
    {
        while (true)
        {
            var stats = GetStats();
            
            if (stats.GlobalQueueSize == 0 && stats.TotalLocalQueueSize == 0)
            {
                // Give workers a moment to finish current work
                await Task.Delay(100);
                
                // Check again to ensure no new work was added
                stats = GetStats();
                if (stats.GlobalQueueSize == 0 && stats.TotalLocalQueueSize == 0)
                {
                    break;
                }
            }
            
            await Task.Delay(50);
        }
    }

    public void Dispose()
    {
        if (_isDisposed)
        {
            return;
        }

        _isDisposed = true;
        Stop();

        foreach (var worker in _workers)
        {
            if (worker is IDisposable disposableWorker)
            {
                disposableWorker.Dispose();
            }
        }

        _workers.Clear();
    }
}

/// <summary>
/// Thread pool statistics.
/// </summary>
public class ThreadPoolStats
{
    /// <summary>
    /// Total number of workers.
    /// </summary>
    public int WorkerCount { get; set; }

    /// <summary>
    /// Number of currently running workers.
    /// </summary>
    public int RunningWorkers { get; set; }

    /// <summary>
    /// Number of items in the global queue.
    /// </summary>
    public int GlobalQueueSize { get; set; }

    /// <summary>
    /// Sizes of each worker's local queue.
    /// </summary>
    public List<int> LocalQueueSizes { get; set; } = new();

    /// <summary>
    /// Total items across all local queues.
    /// </summary>
    public int TotalLocalQueueSize { get; set; }
}
