using IgniteBT.Core.Database;

namespace IgniteBT.Core.Database;

/// <summary>
/// Build.db facade — delegates to SQLite backend while preserving the existing API.
/// </summary>
public sealed class BuildDb : IDisposable
{
    private readonly SqliteBuildDatabase _sqlite;
    private volatile bool _disposed;

    public BuildDb(string databaseDirectory) => _sqlite = new SqliteBuildDatabase(databaseDirectory);

    public void SetModuleHash(string module, string hash) => _sqlite.SetModuleHash(module, hash);
    public bool TryGetModuleHash(string module, out string hash) => _sqlite.TryGetModuleHash(module, out hash!);
    public void SetObjectHash(string sourceFile, string hash) => _sqlite.SetObjectHash(sourceFile, hash);
    public bool TryGetObjectHash(string sourceFile, out string hash) => _sqlite.TryGetObjectHash(sourceFile, out hash!);
    public void SetIncludeGraph(string sourceFile, List<string> headers) => _sqlite.SetIncludeGraph(sourceFile, headers);
    public bool TryGetIncludeGraph(string sourceFile, out List<string> headers) => _sqlite.TryGetIncludeGraph(sourceFile, out headers!);
    public void SetCommandHash(string key, string hash) => _sqlite.SetCommandHash(key, hash);
    public void RecordCompileTime(string sourcePath, string moduleName, long compileTimeMs) =>
        _sqlite.RecordCompileTime(sourcePath, moduleName, compileTimeMs);
    public long GetAverageCompileTime(string sourcePath) => _sqlite.GetAverageCompileTime(sourcePath);
    public void SetCompilerInfo(string type, string version, string path) =>
        _sqlite.SetCompilerInfo(type, version, path);
    public void IncrementCacheStat(string key, bool hit) => _sqlite.IncrementCacheStat(key, hit);
    public DatabaseHealth GetHealth() => _sqlite.GetHealth();
    public void Save() { /* SQLite auto-persists via WAL */ }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        _sqlite.Dispose();
    }
}
