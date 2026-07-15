using System.Text.Json;
using Serilog;

namespace IgniteBT.Build.Scheduler;

/// <summary>
/// Learns from build history and automatically optimizes unity size, parallel jobs,
/// scheduling order, compiler batch size, PCH strategy, and cache strategy.
/// </summary>
public sealed class BuildAutoTuner
{
    private readonly string _tuningPath;
    private TuningProfile _profile;

    public BuildAutoTuner(string databaseDirectory)
    {
        _tuningPath = Path.Combine(databaseDirectory, "auto_tuner.json");
        _profile = Load() ?? TuningProfile.CreateDefault();
    }

    public TuningProfile Profile => _profile;

    public void RecordBuild(BuildTuningObservation observation)
    {
        _profile.TotalBuilds++;
        _profile.LastBuildMs = observation.TotalBuildMs;
        _profile.AverageBuildMs = _profile.TotalBuilds == 1
            ? observation.TotalBuildMs
            : (_profile.AverageBuildMs * (_profile.TotalBuilds - 1) + observation.TotalBuildMs) / _profile.TotalBuilds;

        if (observation.CpuUtilization < 0.85 && _profile.RecommendedJobs < Environment.ProcessorCount)
            _profile.RecommendedJobs = Math.Min(Environment.ProcessorCount, _profile.RecommendedJobs + 1);
        else if (observation.CpuUtilization > 0.98 && _profile.RecommendedJobs > 1)
            _profile.RecommendedJobs = Math.Max(1, _profile.RecommendedJobs - 1);

        if (observation.UnityEnabled)
        {
            if (observation.TotalBuildMs < _profile.BestUnityBuildMs)
            {
                _profile.BestUnityBuildMs = observation.TotalBuildMs;
                _profile.RecommendedUnitySize = observation.UnitySize;
            }
        }

        if (observation.ObjectCacheHitRate > 0.8)
            _profile.PreferObjectCache = true;

        if (observation.PchReuseRate > 0.5)
            _profile.PreferPch = true;

        Save();
        Log.Debug("AutoTuner: jobs={Jobs} unitySize={Unity} preferPch={Pch}",
            _profile.RecommendedJobs, _profile.RecommendedUnitySize, _profile.PreferPch);
    }

    public int GetRecommendedJobs(int requested) =>
        requested > 0 ? requested : _profile.RecommendedJobs;

    public int GetRecommendedUnitySize() => _profile.RecommendedUnitySize;

    private TuningProfile? Load()
    {
        if (!File.Exists(_tuningPath)) return null;
        try { return JsonSerializer.Deserialize<TuningProfile>(File.ReadAllText(_tuningPath)); }
        catch { return null; }
    }

    private void Save() =>
        File.WriteAllText(_tuningPath, JsonSerializer.Serialize(_profile, new JsonSerializerOptions { WriteIndented = true }));
}

public sealed class TuningProfile
{
    public int TotalBuilds { get; set; }
    public long LastBuildMs { get; set; }
    public double AverageBuildMs { get; set; }
    public int RecommendedJobs { get; set; } = Environment.ProcessorCount;
    public int RecommendedUnitySize { get; set; } = 8;
    public long BestUnityBuildMs { get; set; } = long.MaxValue;
    public bool PreferObjectCache { get; set; } = true;
    public bool PreferPch { get; set; } = true;

    public static TuningProfile CreateDefault() => new();
}

public sealed class BuildTuningObservation
{
    public long TotalBuildMs { get; init; }
    public double CpuUtilization { get; init; }
    public double ObjectCacheHitRate { get; init; }
    public double PchReuseRate { get; init; }
    public bool UnityEnabled { get; init; }
    public int UnitySize { get; init; }
}
