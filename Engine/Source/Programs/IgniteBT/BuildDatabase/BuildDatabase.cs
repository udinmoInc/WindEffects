using System.Text.Json;
using Serilog;
using IgniteBT.Compiler;

namespace IgniteBT.BuildDatabase;

/// <summary>
/// Build record for a module.
/// </summary>
public class ModuleBuildRecord
{
    /// <summary>
    /// Module name.
    /// </summary>
    public string ModuleName { get; set; } = string.Empty;

    /// <summary>
    /// Last build configuration.
    /// </summary>
    public BuildConfiguration Configuration { get; set; }

    /// <summary>
    /// Last build platform.
    /// </summary>
    public TargetPlatform Platform { get; set; }

    /// <summary>
    /// Last build time.
    /// </summary>
    public DateTime LastBuildTime { get; set; }

    /// <summary>
    /// Hash of the Build.cs file.
    /// </summary>
    public string BuildCsHash { get; set; } = string.Empty;

    /// <summary>
    /// Compiler version used.
    /// </summary>
    public string CompilerVersion { get; set; } = string.Empty;

    /// <summary>
    /// Whether the build succeeded.
    /// </summary>
    public bool BuildSucceeded { get; set; }

    /// <summary>
    /// Build duration in milliseconds.
    /// </summary>
    public long BuildDurationMs { get; set; }

    /// <summary>
    /// Source files and their hashes.
    /// </summary>
    public Dictionary<string, string> SourceFileHashes { get; set; } = new();

    /// <summary>
    /// Object files produced.
    /// </summary>
    public List<string> ObjectFiles { get; set; } = new();

    /// <summary>
    /// Output file path.
    /// </summary>
    public string OutputFile { get; set; } = string.Empty;
}

/// <summary>
/// Global build metadata.
/// </summary>
public class BuildMetadata
{
    /// <summary>
    /// Database version.
    /// </summary>
    public string Version { get; set; } = "1.0";

    /// <summary>
    /// Last build time.
    /// </summary>
    public DateTime LastBuildTime { get; set; }

    /// <summary>
    /// Total builds performed.
    /// </summary>
    public int TotalBuilds { get; set; }

    /// <summary>
    /// Successful builds.
    /// </summary>
    public int SuccessfulBuilds { get; set; }

    /// <summary>
    /// Failed builds.
    /// </summary>
    public int FailedBuilds { get; set; }

    /// <summary>
    /// Engine root directory.
    /// </summary>
    public string EngineRoot { get; set; } = string.Empty;

    /// <summary>
    /// IgniteBT version.
    /// </summary>
    public string IgniteBTVersion { get; set; } = "1.0.0";
}

/// <summary>
/// Persistent build database for storing build metadata.
/// </summary>
public class BuildDatabase : IDisposable
{
    private readonly string _databasePath;
    private readonly string _modulesPath;
    private readonly Dictionary<string, ModuleBuildRecord> _moduleRecords = new();
    private BuildMetadata _metadata = new();
    private readonly JsonSerializerOptions _jsonOptions;
    private volatile bool _isDisposed;

    /// <summary>
    /// Gets the build metadata.
    /// </summary>
    public BuildMetadata Metadata => _metadata;

    /// <summary>
    /// Creates a new build database.
    /// </summary>
    public BuildDatabase(string databaseDirectory)
    {
        _databasePath = Path.Combine(databaseDirectory, "build_database.json");
        _modulesPath = Path.Combine(databaseDirectory, "modules");
        
        Directory.CreateDirectory(databaseDirectory);
        Directory.CreateDirectory(_modulesPath);

        _jsonOptions = new JsonSerializerOptions
        {
            WriteIndented = true,
            PropertyNamingPolicy = JsonNamingPolicy.CamelCase
        };

        Load();
        
        Log.Information("Build database loaded from {Path}", _databasePath);
    }

    /// <summary>
    /// Loads the database from disk.
    /// </summary>
    private void Load()
    {
        if (!File.Exists(_databasePath))
        {
            Log.Information("No existing build database found, creating new one");
            Save();
            return;
        }

        try
        {
            var json = File.ReadAllText(_databasePath);
            var data = JsonSerializer.Deserialize<DatabaseData>(json, _jsonOptions);
            
            if (data != null)
            {
                _metadata = data.Metadata ?? new BuildMetadata();
                _moduleRecords.Clear();
                
                foreach (var kvp in data.ModuleRecords ?? new Dictionary<string, ModuleBuildRecord>())
                {
                    _moduleRecords[kvp.Key] = kvp.Value;
                }
                
                Log.Information("Loaded {Count} module records from database", _moduleRecords.Count);
            }
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to load build database, starting fresh");
            _metadata = new BuildMetadata();
            _moduleRecords.Clear();
        }
    }

    /// <summary>
    /// Saves the database to disk.
    /// </summary>
    public void Save()
    {
        if (_isDisposed)
        {
            return;
        }

        try
        {
            var data = new DatabaseData
            {
                Metadata = _metadata,
                ModuleRecords = _moduleRecords
            };

            var json = JsonSerializer.Serialize(data, _jsonOptions);
            File.WriteAllText(_databasePath, json);
            
            Log.Debug("Build database saved to {Path}", _databasePath);
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Failed to save build database");
        }
    }

    /// <summary>
    /// Gets or creates a module build record.
    /// </summary>
    public ModuleBuildRecord GetOrCreateModuleRecord(string moduleName)
    {
        if (!_moduleRecords.TryGetValue(moduleName, out var record))
        {
            record = new ModuleBuildRecord
            {
                ModuleName = moduleName
            };
            _moduleRecords[moduleName] = record;
        }
        return record;
    }

    /// <summary>
    /// Updates a module build record.
    /// </summary>
    public void UpdateModuleRecord(ModuleBuildRecord record)
    {
        _moduleRecords[record.ModuleName] = record;
        Save();
    }

    /// <summary>
    /// Gets a module build record if it exists.
    /// </summary>
    public bool TryGetModuleRecord(string moduleName, out ModuleBuildRecord? record)
    {
        return _moduleRecords.TryGetValue(moduleName, out record);
    }

    /// <summary>
    /// Removes a module build record.
    /// </summary>
    public void RemoveModuleRecord(string moduleName)
    {
        _moduleRecords.Remove(moduleName);
        Save();
    }

    /// <summary>
    /// Records a build start.
    /// </summary>
    public void RecordBuildStart()
    {
        _metadata.LastBuildTime = DateTime.UtcNow;
        _metadata.TotalBuilds++;
        Save();
    }

    /// <summary>
    /// Records a successful build.
    /// </summary>
    public void RecordBuildSuccess()
    {
        _metadata.SuccessfulBuilds++;
        Save();
    }

    /// <summary>
    /// Records a failed build.
    /// </summary>
    public void RecordBuildFailure()
    {
        _metadata.FailedBuilds++;
        Save();
    }

    /// <summary>
    /// Invalidates all module records (e.g., after compiler version change).
    /// </summary>
    public void InvalidateAll()
    {
        _moduleRecords.Clear();
        Save();
        Log.Information("Invalidated all module records");
    }

    /// <summary>
    /// Invalidates records for modules that depend on a changed module.
    /// </summary>
    public void InvalidateDependents(string moduleName, Dictionary<string, List<string>> dependencyGraph)
    {
        var toInvalidate = new HashSet<string>();
        var queue = new Queue<string>();
        queue.Enqueue(moduleName);

        while (queue.Count > 0)
        {
            var current = queue.Dequeue();
            
            if (dependencyGraph.TryGetValue(current, out var dependents))
            {
                foreach (var dependent in dependents)
                {
                    if (toInvalidate.Add(dependent))
                    {
                        queue.Enqueue(dependent);
                    }
                }
            }
        }

        foreach (var module in toInvalidate)
        {
            _moduleRecords.Remove(module);
        }

        Save();
        Log.Information("Invalidated {Count} dependent modules of {Module}", toInvalidate.Count, moduleName);
    }

    /// <summary>
    /// Gets database statistics.
    /// </summary>
    public DatabaseStats GetStats()
    {
        return new DatabaseStats
        {
            TotalModules = _moduleRecords.Count,
            TotalBuilds = _metadata.TotalBuilds,
            SuccessfulBuilds = _metadata.SuccessfulBuilds,
            FailedBuilds = _metadata.FailedBuilds,
            LastBuildTime = _metadata.LastBuildTime,
            SuccessRate = _metadata.TotalBuilds > 0 
                ? (double)_metadata.SuccessfulBuilds / _metadata.TotalBuilds 
                : 0.0
        };
    }

    /// <summary>
    /// Clears all data from the database.
    /// </summary>
    public void Clear()
    {
        _moduleRecords.Clear();
        _metadata = new BuildMetadata();
        Save();
        Log.Information("Build database cleared");
    }

    public void Dispose()
    {
        if (_isDisposed)
        {
            return;
        }

        _isDisposed = true;
        Save();
        Log.Information("Build database disposed");
    }
}

/// <summary>
/// Internal data structure for JSON serialization.
/// </summary>
internal class DatabaseData
{
    public BuildMetadata? Metadata { get; set; }
    public Dictionary<string, ModuleBuildRecord>? ModuleRecords { get; set; }
}

/// <summary>
/// Database statistics.
/// </summary>
public class DatabaseStats
{
    /// <summary>
    /// Total number of modules in database.
    /// </summary>
    public int TotalModules { get; set; }

    /// <summary>
    /// Total number of builds recorded.
    /// </summary>
    public int TotalBuilds { get; set; }

    /// <summary>
    /// Number of successful builds.
    /// </summary>
    public int SuccessfulBuilds { get; set; }

    /// <summary>
    /// Number of failed builds.
    /// </summary>
    public int FailedBuilds { get; set; }

    /// <summary>
    /// Last build time.
    /// </summary>
    public DateTime LastBuildTime { get; set; }

    /// <summary>
    /// Build success rate (0.0 to 1.0).
    /// </summary>
    public double SuccessRate { get; set; }
}
