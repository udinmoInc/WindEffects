// ==============================================================================
// WindEffects — IgniteBT — SqliteBuildDatabase
// Source file for the IgniteBT module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using Microsoft.Data.Sqlite;
using Serilog;

namespace IgniteBT.Core.Database;

/// <summary>
/// SQLite-backed build database — files, objects, headers, commands, modules, compiler, SDK, manifest, cache.
/// Replaces binary Build.db with indexed relational storage for enterprise-scale projects.
/// </summary>
public sealed class SqliteBuildDatabase : IDisposable
{
    private readonly SqliteConnection _connection;
    private readonly object _connectionLock = new();
    private volatile bool _disposed;

    public SqliteBuildDatabase(string databaseDirectory)
    {
        Directory.CreateDirectory(databaseDirectory);
        var dbPath = Path.Combine(databaseDirectory, "Build.db");
        _connection = new SqliteConnection($"Data Source={dbPath};Cache=Shared;Mode=ReadWriteCreate");
        _connection.Open();
        InitializeSchema();
        MigrateFromLegacyBinary(dbPath);
    }

    private void InitializeSchema()
    {
        ExecuteBatch("""
            PRAGMA journal_mode=WAL;
            PRAGMA synchronous=NORMAL;
            PRAGMA cache_size=-64000;
            PRAGMA temp_store=MEMORY;

            CREATE TABLE IF NOT EXISTS files (
                path TEXT PRIMARY KEY,
                hash TEXT NOT NULL,
                size INTEGER,
                mtime_ticks INTEGER,
                compile_time_ms INTEGER DEFAULT 0
            );

            CREATE TABLE IF NOT EXISTS objects (
                source_path TEXT PRIMARY KEY,
                hash TEXT NOT NULL,
                cas_key TEXT,
                object_path TEXT,
                compile_time_ms INTEGER DEFAULT 0
            );

            CREATE TABLE IF NOT EXISTS headers (
                header_path TEXT PRIMARY KEY,
                hash TEXT NOT NULL
            );

            CREATE TABLE IF NOT EXISTS header_deps (
                source_path TEXT NOT NULL,
                header_path TEXT NOT NULL,
                PRIMARY KEY (source_path, header_path)
            );

            CREATE TABLE IF NOT EXISTS commands (
                key TEXT PRIMARY KEY,
                hash TEXT NOT NULL,
                command_line TEXT
            );

            CREATE TABLE IF NOT EXISTS modules (
                name TEXT PRIMARY KEY,
                hash TEXT NOT NULL,
                build_cs_path TEXT,
                last_built_ticks INTEGER
            );

            CREATE TABLE IF NOT EXISTS compiler (
                id INTEGER PRIMARY KEY CHECK (id = 1),
                type TEXT,
                version TEXT,
                path TEXT,
                detected_utc TEXT
            );

            CREATE TABLE IF NOT EXISTS sdk (
                name TEXT PRIMARY KEY,
                version TEXT,
                hash TEXT,
                root_path TEXT
            );

            CREATE TABLE IF NOT EXISTS manifest (
                id INTEGER PRIMARY KEY CHECK (id = 1),
                config_hash TEXT,
                toolchain_hash TEXT,
                graph_hash TEXT,
                built_utc TEXT
            );

            CREATE TABLE IF NOT EXISTS cache_stats (
                key TEXT PRIMARY KEY,
                hits INTEGER DEFAULT 0,
                misses INTEGER DEFAULT 0,
                last_updated_utc TEXT
            );

            CREATE TABLE IF NOT EXISTS compile_history (
                source_path TEXT NOT NULL,
                module_name TEXT,
                compile_time_ms INTEGER,
                header_count INTEGER DEFAULT 0,
                pch_used INTEGER DEFAULT 0,
                pch_name TEXT DEFAULT '',
                recorded_utc TEXT,
                PRIMARY KEY (source_path, recorded_utc)
            );

            CREATE TABLE IF NOT EXISTS source_signatures (
                file_path TEXT PRIMARY KEY,
                content_hash TEXT NOT NULL,
                symbol_hash TEXT NOT NULL,
                include_hash TEXT NOT NULL,
                functions_summary TEXT,
                classes_summary TEXT,
                compiler_identity TEXT,
                pch_identity TEXT,
                object_identity TEXT,
                last_classified_utc TEXT
            );

            CREATE INDEX IF NOT EXISTS idx_header_deps_header ON header_deps(header_path);
            CREATE INDEX IF NOT EXISTS idx_objects_cas ON objects(cas_key);
            CREATE INDEX IF NOT EXISTS idx_compile_history_source ON compile_history(source_path);
            CREATE INDEX IF NOT EXISTS idx_compile_history_module ON compile_history(module_name);
            CREATE INDEX IF NOT EXISTS idx_source_signatures_file ON source_signatures(file_path);
            """);

        try { ExecuteBatch("ALTER TABLE compile_history ADD COLUMN header_count INTEGER DEFAULT 0;"); } catch { }
        try { ExecuteBatch("ALTER TABLE compile_history ADD COLUMN pch_used INTEGER DEFAULT 0;"); } catch { }
        try { ExecuteBatch("ALTER TABLE compile_history ADD COLUMN pch_name TEXT DEFAULT '';"); } catch { }
    }

    public void SetModuleHash(string module, string hash)
    {
        lock (_connectionLock)
            Upsert("modules", "name", module, ("hash", hash), ("last_built_ticks", DateTime.UtcNow.Ticks));
    }

    public bool TryGetModuleHash(string module, out string hash)
    {
        lock (_connectionLock)
        {
            hash = QueryScalar("SELECT hash FROM modules WHERE name = $n", ("$n", module)) ?? string.Empty;
            return hash.Length > 0;
        }
    }

    public void SetObjectHash(string sourceFile, string hash, string? casKey = null, string? objectPath = null)
    {
        lock (_connectionLock)
        {
        using var cmd = _connection.CreateCommand();
        cmd.CommandText = """
            INSERT INTO objects (source_path, hash, cas_key, object_path) VALUES ($s, $h, $c, $o)
            ON CONFLICT(source_path) DO UPDATE SET hash=$h, cas_key=$c, object_path=$o
            """;
        cmd.Parameters.AddWithValue("$s", sourceFile);
        cmd.Parameters.AddWithValue("$h", hash);
        cmd.Parameters.AddWithValue("$c", casKey ?? (object)DBNull.Value);
        cmd.Parameters.AddWithValue("$o", objectPath ?? (object)DBNull.Value);
        cmd.ExecuteNonQuery();
        }
    }

    public bool TryGetObjectHash(string sourceFile, out string hash)
    {
        lock (_connectionLock)
        {
            hash = QueryScalar("SELECT hash FROM objects WHERE source_path = $s", ("$s", sourceFile)) ?? string.Empty;
            return hash.Length > 0;
        }
    }

    public void SetIncludeGraph(string sourceFile, List<string> headers)
    {
        lock (_connectionLock)
        {
        using var tx = _connection.BeginTransaction();
        using (var del = _connection.CreateCommand())
        {
            del.CommandText = "DELETE FROM header_deps WHERE source_path = $s";
            del.Parameters.AddWithValue("$s", sourceFile);
            del.Transaction = tx;
            del.ExecuteNonQuery();
        }
        foreach (var header in headers)
        {
            using var ins = _connection.CreateCommand();
            ins.CommandText = "INSERT OR IGNORE INTO header_deps (source_path, header_path) VALUES ($s, $h)";
            ins.Parameters.AddWithValue("$s", sourceFile);
            ins.Parameters.AddWithValue("$h", header);
            ins.Transaction = tx;
            ins.ExecuteNonQuery();
        }
        tx.Commit();
        }
    }

    public bool TryGetIncludeGraph(string sourceFile, out List<string> headers)
    {
        lock (_connectionLock)
        {
            headers = new List<string>();
            using var cmd = _connection.CreateCommand();
            cmd.CommandText = "SELECT header_path FROM header_deps WHERE source_path = $s";
            cmd.Parameters.AddWithValue("$s", sourceFile);
            using var reader = cmd.ExecuteReader();
            while (reader.Read()) headers.Add(reader.GetString(0));
            return headers.Count > 0;
        }
    }

    public List<string> GetTUsIncludingHeader(string headerPath)
    {
        lock (_connectionLock)
        {
            var tus = new List<string>();
            using var cmd = _connection.CreateCommand();
            cmd.CommandText = "SELECT DISTINCT source_path FROM header_deps WHERE header_path = $h OR header_path LIKE $hLike";
            cmd.Parameters.AddWithValue("$h", headerPath);
            cmd.Parameters.AddWithValue("$hLike", "%" + Path.GetFileName(headerPath));
            using var reader = cmd.ExecuteReader();
            while (reader.Read()) tus.Add(reader.GetString(0));
            return tus;
        }
    }

    public List<string> GetTransitiveIncludeTUs(string headerPath)
    {
        lock (_connectionLock)
        {
            var tus = new List<string>();
            using var cmd = _connection.CreateCommand();
            cmd.CommandText = """
                WITH RECURSIVE DependentHeaders(h_path) AS (
                    SELECT $h
                    UNION
                    SELECT hd.header_path
                    FROM header_deps hd
                    JOIN DependentHeaders dh ON hd.source_path = dh.h_path
                )
                SELECT DISTINCT hd.source_path
                FROM header_deps hd
                WHERE hd.header_path IN (SELECT h_path FROM DependentHeaders)
                   OR hd.header_path LIKE $hLike;
                """;
            cmd.Parameters.AddWithValue("$h", headerPath);
            cmd.Parameters.AddWithValue("$hLike", "%" + Path.GetFileName(headerPath));
            using var reader = cmd.ExecuteReader();
            while (reader.Read()) tus.Add(reader.GetString(0));
            return tus;
        }
    }

    public void UpsertSourceSignature(SourceSignatureRecord record)
    {
        lock (_connectionLock)
        {
            using var cmd = _connection.CreateCommand();
            cmd.CommandText = """
                INSERT INTO source_signatures (file_path, content_hash, symbol_hash, include_hash, functions_summary, classes_summary, compiler_identity, pch_identity, object_identity, last_classified_utc)
                VALUES ($f, $c, $s, $i, $fn, $cl, $ci, $pi, $oi, $u)
                ON CONFLICT(file_path) DO UPDATE SET content_hash=$c, symbol_hash=$s, include_hash=$i, functions_summary=$fn, classes_summary=$cl, compiler_identity=$ci, pch_identity=$pi, object_identity=$oi, last_classified_utc=$u;
                """;
            cmd.Parameters.AddWithValue("$f", record.FilePath);
            cmd.Parameters.AddWithValue("$c", record.ContentHash);
            cmd.Parameters.AddWithValue("$s", record.SymbolHash);
            cmd.Parameters.AddWithValue("$i", record.IncludeHash);
            cmd.Parameters.AddWithValue("$fn", record.FunctionsSummary);
            cmd.Parameters.AddWithValue("$cl", record.ClassesSummary);
            cmd.Parameters.AddWithValue("$ci", record.CompilerIdentity);
            cmd.Parameters.AddWithValue("$pi", record.PchIdentity);
            cmd.Parameters.AddWithValue("$oi", record.ObjectIdentity);
            cmd.Parameters.AddWithValue("$u", record.LastClassifiedUtc);
            cmd.ExecuteNonQuery();
        }
    }

    public bool TryGetSourceSignature(string filePath, out SourceSignatureRecord? record)
    {
        lock (_connectionLock)
        {
            record = null;
            using var cmd = _connection.CreateCommand();
            cmd.CommandText = "SELECT file_path, content_hash, symbol_hash, include_hash, functions_summary, classes_summary, compiler_identity, pch_identity, object_identity, last_classified_utc FROM source_signatures WHERE file_path = $f";
            cmd.Parameters.AddWithValue("$f", filePath);
            using var reader = cmd.ExecuteReader();
            if (reader.Read())
            {
                record = new SourceSignatureRecord
                {
                    FilePath = reader.GetString(0),
                    ContentHash = reader.GetString(1),
                    SymbolHash = reader.GetString(2),
                    IncludeHash = reader.GetString(3),
                    FunctionsSummary = reader.IsDBNull(4) ? "" : reader.GetString(4),
                    ClassesSummary = reader.IsDBNull(5) ? "" : reader.GetString(5),
                    CompilerIdentity = reader.IsDBNull(6) ? "" : reader.GetString(6),
                    PchIdentity = reader.IsDBNull(7) ? "" : reader.GetString(7),
                    ObjectIdentity = reader.IsDBNull(8) ? "" : reader.GetString(8),
                    LastClassifiedUtc = reader.IsDBNull(9) ? "" : reader.GetString(9)
                };
                return true;
            }
            return false;
        }
    }

    public void SetCommandHash(string key, string hash)
    {
        lock (_connectionLock)
            Upsert("commands", "key", key, ("hash", hash));
    }

    public void RecordCompileTime(string sourcePath, string moduleName, long compileTimeMs)
    {
        RecordTuCostMetrics(sourcePath, moduleName, compileTimeMs, 0, false, null);
    }

    public void RecordTuCostMetrics(string sourcePath, string moduleName, long compileTimeMs, int headerCount, bool pchUsed, string? pchName)
    {
        lock (_connectionLock)
        {
        using var cmd = _connection.CreateCommand();
        cmd.CommandText = """
            INSERT INTO compile_history (source_path, module_name, compile_time_ms, header_count, pch_used, pch_name, recorded_utc)
            VALUES ($s, $m, $t, $hc, $pu, $pn, $u)
            """;
        cmd.Parameters.AddWithValue("$s", sourcePath);
        cmd.Parameters.AddWithValue("$m", moduleName);
        cmd.Parameters.AddWithValue("$t", compileTimeMs);
        cmd.Parameters.AddWithValue("$hc", headerCount);
        cmd.Parameters.AddWithValue("$pu", pchUsed ? 1 : 0);
        cmd.Parameters.AddWithValue("$pn", pchName ?? string.Empty);
        cmd.Parameters.AddWithValue("$u", DateTime.UtcNow.ToString("O"));
        cmd.ExecuteNonQuery();

        using var upd = _connection.CreateCommand();
        upd.CommandText = "UPDATE files SET compile_time_ms = $t WHERE path = $s";
        upd.Parameters.AddWithValue("$s", sourcePath);
        upd.Parameters.AddWithValue("$t", compileTimeMs);
        upd.ExecuteNonQuery();
        }
    }

    public long GetAverageCompileTime(string sourcePath)
    {
        lock (_connectionLock)
        {
            var val = QueryScalar(
                "SELECT AVG(compile_time_ms) FROM compile_history WHERE source_path = $s",
                ("$s", sourcePath));
            return long.TryParse(val, out var ms) ? ms : 0;
        }
    }

    public TuCostStats GetTuCostStats(string sourcePath)
    {
        lock (_connectionLock)
        {
            using var cmd = _connection.CreateCommand();
            cmd.CommandText = """
                SELECT compile_time_ms, header_count, pch_used, pch_name
                FROM compile_history WHERE source_path = $s
                ORDER BY recorded_utc ASC
                """;
            cmd.Parameters.AddWithValue("$s", sourcePath);
            using var reader = cmd.ExecuteReader();
            var times = new List<long>();
            int lastHeaderCount = 0;
            bool lastPchUsed = false;
            string lastPchName = string.Empty;
            while (reader.Read())
            {
                times.Add(reader.GetInt64(0));
                lastHeaderCount = reader.GetInt32(1);
                lastPchUsed = reader.GetInt32(2) == 1;
                lastPchName = reader.GetString(3);
            }
            if (times.Count == 0) return new TuCostStats { SourcePath = sourcePath };

            times.Sort();
            long avg = (long)times.Average();
            long min = times.First();
            long max = times.Last();
            long median = times[times.Count / 2];
            int p95Idx = (int)Math.Ceiling(times.Count * 0.95) - 1;
            long p95 = times[Math.Clamp(p95Idx, 0, times.Count - 1)];

            string trend = "STABLE";
            if (times.Count >= 2)
            {
                var recent = times.TakeLast(Math.Max(1, times.Count / 2)).Average();
                var older = times.Take(Math.Max(1, times.Count / 2)).Average();
                if (recent < older * 0.9) trend = $"-{(1.0 - recent / older):P0}";
                else if (recent > older * 1.1) trend = $"+{(recent / older - 1.0):P0}";
            }

            return new TuCostStats
            {
                SourcePath = sourcePath,
                AverageMs = avg,
                MedianMs = median,
                P95Ms = p95,
                MinMs = min,
                MaxMs = max,
                RunCount = times.Count,
                HeaderCount = lastHeaderCount,
                PchUsed = lastPchUsed,
                PchName = lastPchName,
                Trend = trend
            };
        }
    }

    public List<TuCostRecord> GetTopSlowestTUs(int count = 20)
    {
        lock (_connectionLock)
        {
            var results = new List<TuCostRecord>();
            using var cmd = _connection.CreateCommand();
            cmd.CommandText = """
                SELECT source_path, module_name, AVG(compile_time_ms) as avg_ms, MAX(compile_time_ms) as max_ms,
                       MAX(header_count) as hc, MAX(pch_used) as pu, MAX(pch_name) as pn, COUNT(*) as cnt
                FROM compile_history
                GROUP BY source_path, module_name
                ORDER BY avg_ms DESC
                LIMIT $c
                """;
            cmd.Parameters.AddWithValue("$c", count);
            using var reader = cmd.ExecuteReader();
            while (reader.Read())
            {
                results.Add(new TuCostRecord
                {
                    SourcePath = reader.GetString(0),
                    ModuleName = reader.IsDBNull(1) ? "Unknown" : reader.GetString(1),
                    AverageMs = Convert.ToInt64(reader.GetDouble(2)),
                    MaxMs = reader.GetInt64(3),
                    HeaderCount = reader.GetInt32(4),
                    PchUsed = reader.GetInt32(5) == 1,
                    PchName = reader.GetString(6),
                    RunCount = reader.GetInt32(7)
                });
            }
            return results;
        }
    }

    public void SetCompilerInfo(string type, string version, string path)
    {
        lock (_connectionLock)
        {
        using var cmd = _connection.CreateCommand();
        cmd.CommandText = """
            INSERT INTO compiler (id, type, version, path, detected_utc) VALUES (1, $t, $v, $p, $u)
            ON CONFLICT(id) DO UPDATE SET type=$t, version=$v, path=$p, detected_utc=$u
            """;
        cmd.Parameters.AddWithValue("$t", type);
        cmd.Parameters.AddWithValue("$v", version);
        cmd.Parameters.AddWithValue("$p", path);
        cmd.Parameters.AddWithValue("$u", DateTime.UtcNow.ToString("O"));
        cmd.ExecuteNonQuery();
        }
    }

    public void IncrementCacheStat(string key, bool hit)
    {
        lock (_connectionLock)
        {
        using var cmd = _connection.CreateCommand();
        cmd.CommandText = hit
            ? """
              INSERT INTO cache_stats (key, hits, misses, last_updated_utc) VALUES ($k, 1, 0, $u)
              ON CONFLICT(key) DO UPDATE SET hits = hits + 1, last_updated_utc = $u
              """
            : """
              INSERT INTO cache_stats (key, hits, misses, last_updated_utc) VALUES ($k, 0, 1, $u)
              ON CONFLICT(key) DO UPDATE SET misses = misses + 1, last_updated_utc = $u
              """;
        cmd.Parameters.AddWithValue("$k", key);
        cmd.Parameters.AddWithValue("$u", DateTime.UtcNow.ToString("O"));
        cmd.ExecuteNonQuery();
        }
    }

    public DatabaseHealth GetHealth()
    {
        lock (_connectionLock)
        {
            return new DatabaseHealth
            {
                FileCount = CountRows("files"),
                ObjectCount = CountRows("objects"),
                HeaderDepCount = CountRows("header_deps"),
                ModuleCount = CountRows("modules"),
                CompileHistoryCount = CountRows("compile_history"),
                DatabaseSizeBytes = new FileInfo(_connection.DataSource).Length
            };
        }
    }

    private void Upsert(string table, string keyCol, string keyVal, params (string col, object val)[] cols)
    {
        var colNames = string.Join(", ", cols.Select(c => c.col).Prepend(keyCol));
        var paramNames = string.Join(", ", cols.Select(c => "$" + c.col).Prepend("$key"));
        var updates = string.Join(", ", cols.Select(c => $"{c.col}=${c.col}"));

        using var cmd = _connection.CreateCommand();
        cmd.CommandText =
            $"INSERT INTO {table} ({colNames}) VALUES ({paramNames}) ON CONFLICT({keyCol}) DO UPDATE SET {updates}";
        cmd.Parameters.AddWithValue("$key", keyVal);
        foreach (var (col, val) in cols)
            cmd.Parameters.AddWithValue("$" + col, val);
        cmd.ExecuteNonQuery();
    }

    private string? QueryScalar(string sql, params (string param, object val)[] parameters)
    {
        using var cmd = _connection.CreateCommand();
        cmd.CommandText = sql;
        foreach (var (param, val) in parameters)
            cmd.Parameters.AddWithValue(param, val);
        return cmd.ExecuteScalar()?.ToString();
    }

    private long CountRows(string table)
    {
        using var cmd = _connection.CreateCommand();
        cmd.CommandText = $"SELECT COUNT(*) FROM {table}";
        return (long)(cmd.ExecuteScalar() ?? 0L);
    }

    private void ExecuteBatch(string sql)
    {
        using var cmd = _connection.CreateCommand();
        cmd.CommandText = sql;
        cmd.ExecuteNonQuery();
    }

    private void MigrateFromLegacyBinary(string dbPath)
    {
        var legacyPath = dbPath + ".legacy";
        if (!File.Exists(legacyPath) && File.Exists(dbPath))
        {
            // Check if it's legacy binary format
            try
            {
                using var stream = File.OpenRead(dbPath);
                var magic = new byte[8];
                if (stream.Read(magic, 0, 8) == 8 && magic.AsSpan().SequenceEqual("IGBTDB01"u8))
                {
                    File.Move(dbPath, legacyPath);
                    _connection.Close();
                    _connection.Open();
                    ImportLegacy(legacyPath);
                }
            }
            catch { /* already sqlite */ }
        }
    }

    private void ImportLegacy(string legacyPath)
    {
        try
        {
            var legacy = new BuildDbLegacyReader(legacyPath);
            foreach (var (k, v) in legacy.ModuleHashes) SetModuleHash(k, v);
            foreach (var (k, v) in legacy.ObjectHashes) SetObjectHash(k, v);
            foreach (var (k, v) in legacy.IncludeGraph) SetIncludeGraph(k, v);
            foreach (var (k, v) in legacy.CommandHashes) SetCommandHash(k, v);
            Log.Information("Migrated legacy Build.db to SQLite");
        }
        catch (Exception ex) { Log.Warning(ex, "Legacy Build.db migration skipped"); }
    }

    public void Dispose()
    {
        if (_disposed) return;
        lock (_connectionLock)
        {
            if (_disposed) return;
            _disposed = true;
            _connection.Dispose();
        }
    }
}

public sealed class DatabaseHealth
{
    public long FileCount { get; init; }
    public long ObjectCount { get; init; }
    public long HeaderDepCount { get; init; }
    public long ModuleCount { get; init; }
    public long CompileHistoryCount { get; init; }
    public long DatabaseSizeBytes { get; init; }
}

public sealed class TuCostStats
{
    public string SourcePath { get; init; } = string.Empty;
    public long AverageMs { get; init; }
    public long MedianMs { get; init; }
    public long P95Ms { get; init; }
    public long MinMs { get; init; }
    public long MaxMs { get; init; }
    public int RunCount { get; init; }
    public int HeaderCount { get; init; }
    public bool PchUsed { get; init; }
    public string PchName { get; init; } = string.Empty;
    public string Trend { get; init; } = "STABLE";
}

public sealed class TuCostRecord
{
    public string SourcePath { get; init; } = string.Empty;
    public string ModuleName { get; init; } = string.Empty;
    public long AverageMs { get; init; }
    public long MaxMs { get; init; }
    public int HeaderCount { get; init; }
    public bool PchUsed { get; init; }
    public string PchName { get; init; } = string.Empty;
    public int RunCount { get; init; }
}

/// <summary>Reads legacy binary Build.db for one-time migration.</summary>
internal sealed class BuildDbLegacyReader
{
    public Dictionary<string, string> ModuleHashes { get; } = new();
    public Dictionary<string, string> ObjectHashes { get; } = new();
    public Dictionary<string, List<string>> IncludeGraph { get; } = new();
    public Dictionary<string, string> CommandHashes { get; } = new();

    public BuildDbLegacyReader(string path)
    {
        using var stream = File.OpenRead(path);
        var magic = new byte[8];
        stream.Read(magic, 0, 8);
        var lenBytes = new byte[4];
        stream.Read(lenBytes, 0, 4);
        var len = System.Buffers.Binary.BinaryPrimitives.ReadInt32LittleEndian(lenBytes);
        stream.Seek(16, SeekOrigin.Begin);
        var json = new byte[len];
        stream.Read(json, 0, len);
        var payload = System.Text.Json.JsonSerializer.Deserialize<LegacyPayload>(json);
        if (payload == null) return;
        foreach (var (k, v) in payload.ModuleHashes) ModuleHashes[k] = v;
        foreach (var (k, v) in payload.ObjectHashes) ObjectHashes[k] = v;
        foreach (var (k, v) in payload.IncludeGraph) IncludeGraph[k] = v;
        foreach (var (k, v) in payload.CommandHashes) CommandHashes[k] = v;
    }

    private sealed class LegacyPayload
    {
        public Dictionary<string, string> ModuleHashes { get; set; } = new();
        public Dictionary<string, string> ObjectHashes { get; set; } = new();
        public Dictionary<string, List<string>> IncludeGraph { get; set; } = new();
        public Dictionary<string, string> CommandHashes { get; set; } = new();
    }
}
