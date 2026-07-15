using System.Text;
using System.Text.Json;
using IgniteBT.Core.Hashing;

namespace IgniteBT.Build.Orchestration;

public sealed class FileSnapshot
{
    public string Path { get; set; } = string.Empty;
    public long Size { get; set; }
    public long MtimeTicks { get; set; }
}

/// <summary>
/// Lightweight workspace input snapshot for sub-100ms no-op detection.
/// Validates mtime+size only — no file content hashing on the probe path.
/// </summary>
public sealed class WorkspaceSnapshot
{
    public const string FileName = "workspace_snapshot.json";
    public const string BinaryFileName = "workspace_snapshot.bin";
    private const uint BinaryMagic = 0x4E534749; // "IGSN"
    private const byte BinaryVersion = 2;
    private const byte BinaryVersionLegacy = 1;

    public string Config { get; set; } = string.Empty;
    public string Platform { get; set; } = string.Empty;
    public string? TargetName { get; set; }
    public string BuildFlagsHash { get; set; } = string.Empty;
    public string FeatureFlagsHash { get; set; } = string.Empty;
    public string ManifestConfigHash { get; set; } = string.Empty;
    public string ManifestToolchainHash { get; set; } = string.Empty;
    public string CompilerVersion { get; set; } = string.Empty;
    public string CompilerPath { get; set; } = string.Empty;
    public long CompilerMtimeTicks { get; set; }
    public long CompilerSize { get; set; }
    public int Jobs { get; set; }
    public bool UnityBuild { get; set; }
    public int UnitySize { get; set; }
    public string UnityDisabled { get; set; } = string.Empty;
    public List<string> BuildCsPaths { get; set; } = new();
    public List<FileSnapshot> Files { get; set; } = new();

    public static string GetPath(string databaseDirectory) =>
        Path.Combine(databaseDirectory, FileName);

    public static string GetBinaryPath(string databaseDirectory) =>
        Path.Combine(databaseDirectory, BinaryFileName);

    public static bool TryLoad(string databaseDirectory, out WorkspaceSnapshot? snapshot)
    {
        snapshot = null;
        if (TryLoadBinary(GetBinaryPath(databaseDirectory), out snapshot))
            return true;

        var path = GetPath(databaseDirectory);
        if (!File.Exists(path)) return false;
        try
        {
            snapshot = JsonSerializer.Deserialize<WorkspaceSnapshot>(File.ReadAllText(path));
            if (snapshot != null)
                snapshot.SaveBinary(databaseDirectory);
            return snapshot != null;
        }
        catch
        {
            return false;
        }
    }

    public void Save(string databaseDirectory)
    {
        Directory.CreateDirectory(databaseDirectory);
        SaveBinary(databaseDirectory);
        var json = JsonSerializer.Serialize(this, new JsonSerializerOptions { WriteIndented = false });
        File.WriteAllText(GetPath(databaseDirectory), json);
    }

    public void SaveBinary(string databaseDirectory)
    {
        Directory.CreateDirectory(databaseDirectory);
        using var stream = File.Create(GetBinaryPath(databaseDirectory));
        using var writer = new BinaryWriter(stream, Encoding.UTF8, leaveOpen: false);
        writer.Write(BinaryMagic);
        writer.Write(BinaryVersion);
        WriteString(writer, Config);
        WriteString(writer, Platform);
        WriteString(writer, TargetName ?? string.Empty);
        WriteString(writer, BuildFlagsHash);
        WriteString(writer, FeatureFlagsHash);
        WriteString(writer, ManifestConfigHash);
        WriteString(writer, ManifestToolchainHash);
        WriteString(writer, CompilerVersion);
        WriteString(writer, CompilerPath);
        writer.Write(CompilerMtimeTicks);
        writer.Write(CompilerSize);
        writer.Write(Jobs);
        writer.Write((byte)(UnityBuild ? 1 : 0));
        writer.Write(UnitySize);
        WriteString(writer, UnityDisabled);
        writer.Write(BuildCsPaths.Count);
        foreach (var buildCs in BuildCsPaths)
            WriteString(writer, buildCs);
        writer.Write(Files.Count);
        foreach (var file in Files)
        {
            WriteString(writer, file.Path);
            writer.Write(file.Size);
            writer.Write(file.MtimeTicks);
        }
    }

    private static bool TryLoadBinary(string path, out WorkspaceSnapshot? snapshot)
    {
        snapshot = null;
        if (!File.Exists(path)) return false;

        try
        {
            using var stream = File.OpenRead(path);
            using var reader = new BinaryReader(stream, Encoding.UTF8, leaveOpen: false);
            if (reader.ReadUInt32() != BinaryMagic) return false;
            var version = reader.ReadByte();
            if (version != BinaryVersion && version != BinaryVersionLegacy) return false;

            snapshot = new WorkspaceSnapshot
            {
                Config = ReadString(reader),
                Platform = ReadString(reader),
                TargetName = ReadNullableTarget(ReadString(reader)),
                BuildFlagsHash = ReadString(reader),
                FeatureFlagsHash = ReadString(reader),
                ManifestConfigHash = ReadString(reader),
                ManifestToolchainHash = ReadString(reader),
                CompilerVersion = ReadString(reader),
                CompilerPath = ReadString(reader),
                CompilerMtimeTicks = reader.ReadInt64(),
                CompilerSize = reader.ReadInt64()
            };

            if (version >= BinaryVersion)
            {
                snapshot.Jobs = reader.ReadInt32();
                snapshot.UnityBuild = reader.ReadByte() != 0;
                snapshot.UnitySize = reader.ReadInt32();
                snapshot.UnityDisabled = ReadString(reader);
            }

            var buildCsCount = reader.ReadInt32();
            snapshot.BuildCsPaths = new List<string>(buildCsCount);
            for (var i = 0; i < buildCsCount; i++)
                snapshot.BuildCsPaths.Add(ReadString(reader));

            var fileCount = reader.ReadInt32();
            snapshot.Files = new List<FileSnapshot>(fileCount);
            for (var i = 0; i < fileCount; i++)
            {
                snapshot.Files.Add(new FileSnapshot
                {
                    Path = ReadString(reader),
                    Size = reader.ReadInt64(),
                    MtimeTicks = reader.ReadInt64()
                });
            }

            return true;
        }
        catch
        {
            snapshot = null;
            return false;
        }
    }

    public static WorkspaceSnapshot Create(
        string config,
        string platform,
        string? targetName,
        string buildFlagsHash,
        string featureFlagsHash,
        string manifestConfigHash,
        string manifestToolchainHash,
        string compilerVersion,
        string compilerPath,
        int jobs,
        bool unityBuild,
        int unitySize,
        string unityDisabled,
        IEnumerable<string> buildCsPaths,
        IEnumerable<string> sourcePaths)
    {
        var files = new List<FileSnapshot>();
        foreach (var path in sourcePaths)
        {
            var snap = SnapshotFile(path);
            if (snap != null) files.Add(snap);
        }

        var snapshot = new WorkspaceSnapshot
        {
            Config = config,
            Platform = platform,
            TargetName = targetName,
            BuildFlagsHash = buildFlagsHash,
            FeatureFlagsHash = featureFlagsHash,
            ManifestConfigHash = manifestConfigHash,
            ManifestToolchainHash = manifestToolchainHash,
            CompilerVersion = compilerVersion,
            CompilerPath = compilerPath,
            Jobs = jobs,
            UnityBuild = unityBuild,
            UnitySize = unitySize,
            UnityDisabled = unityDisabled,
            BuildCsPaths = buildCsPaths.OrderBy(p => p, StringComparer.OrdinalIgnoreCase).ToList(),
            Files = files.OrderBy(f => f.Path, StringComparer.OrdinalIgnoreCase).ToList()
        };

        if (!string.IsNullOrEmpty(compilerPath) && File.Exists(compilerPath))
        {
            var info = new FileInfo(compilerPath);
            snapshot.CompilerMtimeTicks = info.LastWriteTimeUtc.Ticks;
            snapshot.CompilerSize = info.Length;
        }

        return snapshot;
    }

    public static FileSnapshot? SnapshotFile(string path)
    {
        if (!File.Exists(path)) return null;
        var info = new FileInfo(path);
        return new FileSnapshot
        {
            Path = Path.GetFullPath(path),
            Size = info.Length,
            MtimeTicks = info.LastWriteTimeUtc.Ticks
        };
    }

    public bool MatchesBuildRequest(
        string config,
        string platform,
        string? targetName,
        string buildFlagsHash,
        int jobs = 0,
        bool unityBuild = false,
        int unitySize = 0,
        string? unityDisabled = null)
    {
        return string.Equals(Config, config, StringComparison.OrdinalIgnoreCase)
            && string.Equals(Platform, platform, StringComparison.OrdinalIgnoreCase)
            && string.Equals(TargetName ?? string.Empty, targetName ?? string.Empty, StringComparison.OrdinalIgnoreCase)
            && string.Equals(BuildFlagsHash, buildFlagsHash, StringComparison.Ordinal)
            && Jobs == jobs
            && UnityBuild == unityBuild
            && UnitySize == unitySize
            && string.Equals(UnityDisabled, unityDisabled ?? string.Empty, StringComparison.Ordinal);
    }

    public bool TryValidate(string engineDir, out string? failureReason)
    {
        failureReason = null;

        if (!string.IsNullOrEmpty(CompilerPath))
        {
            if (!File.Exists(CompilerPath))
            {
                failureReason = "compiler missing";
                return false;
            }

            if (!MatchesFileMetadata(CompilerPath, CompilerSize, CompilerMtimeTicks))
            {
                failureReason = "compiler changed";
                return false;
            }
        }

        foreach (var buildCsPath in BuildCsPaths)
        {
            if (!File.Exists(buildCsPath))
            {
                failureReason = "Build.cs set changed";
                return false;
            }
        }

        var sourceRoot = Path.Combine(engineDir, "Source");
        if (!Directory.Exists(sourceRoot))
        {
            failureReason = "Build.cs set changed";
            return false;
        }

        var buildCsCount = 0;
        foreach (var _ in Directory.EnumerateFiles(sourceRoot, "*.Build.cs", SearchOption.AllDirectories))
        {
            if (++buildCsCount > BuildCsPaths.Count)
                break;
        }

        if (buildCsCount != BuildCsPaths.Count)
        {
            failureReason = "Build.cs set changed";
            return false;
        }

        foreach (var file in Files)
        {
            if (!File.Exists(file.Path))
            {
                failureReason = "input file(s) changed";
                return false;
            }

            if (!MatchesFileMetadata(file.Path, file.Size, file.MtimeTicks))
            {
                failureReason = "input file(s) changed";
                return false;
            }
        }

        return true;
    }

    private static bool MatchesFileMetadata(string path, long expectedSize, long expectedMtimeTicks)
    {
        var info = new FileInfo(path);
        return info.Length == expectedSize && info.LastWriteTimeUtc.Ticks == expectedMtimeTicks;
    }

    private static void WriteString(BinaryWriter writer, string value)
    {
        var bytes = Encoding.UTF8.GetBytes(value);
        writer.Write(bytes.Length);
        writer.Write(bytes);
    }

    private static string ReadString(BinaryReader reader)
    {
        var length = reader.ReadInt32();
        return Encoding.UTF8.GetString(reader.ReadBytes(length));
    }

    private static string? ReadNullableTarget(string value) =>
        string.IsNullOrEmpty(value) ? null : value;
}
