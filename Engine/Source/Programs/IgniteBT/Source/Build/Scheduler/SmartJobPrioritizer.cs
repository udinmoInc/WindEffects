using IgniteBT.Core.Threading;

namespace IgniteBT.Build.Scheduler;

/// <summary>
/// Prioritizes jobs by compile time history, dependency depth, and file size.
/// Largest/slowest/critical-path jobs run first for maximum CPU utilization.
/// </summary>
public sealed class SmartJobPrioritizer
{
    private readonly Dictionary<string, long> _historicalCompileTimes = new(StringComparer.OrdinalIgnoreCase);
    private readonly Dictionary<string, int> _dependencyDepth = new(StringComparer.OrdinalIgnoreCase);

    public void SetHistoricalCompileTime(string jobId, long compileTimeMs) =>
        _historicalCompileTimes[jobId] = compileTimeMs;

    public void SetDependencyDepth(string jobId, int depth) =>
        _dependencyDepth[jobId] = depth;

    public IEnumerable<JobNode> Prioritize(IEnumerable<JobNode> readyNodes)
    {
        return readyNodes
            .OrderByDescending(n => GetPriority(n))
            .ThenByDescending(n => _historicalCompileTimes.GetValueOrDefault(n.Id))
            .ThenByDescending(n => _dependencyDepth.GetValueOrDefault(n.Id));
    }

    private static int GetPriority(JobNode node)
    {
        // Link jobs after compile jobs at same priority tier
        if (node.Id.StartsWith("link:", StringComparison.OrdinalIgnoreCase)) return 1;
        if (node.Id.StartsWith("compile:", StringComparison.OrdinalIgnoreCase)) return 2;
        return 0;
    }
}
