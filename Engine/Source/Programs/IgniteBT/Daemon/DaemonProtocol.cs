using System.Text.Json;
using System.Text.Json.Serialization;

namespace IgniteBT.Daemon;

public sealed class DaemonRequest
{
    [JsonPropertyName("command")]
    public string Command { get; set; } = string.Empty;

    [JsonPropertyName("args")]
    public string[] Args { get; set; } = [];

    [JsonPropertyName("cwd")]
    public string WorkingDirectory { get; set; } = string.Empty;
}

public sealed class DaemonResponse
{
    [JsonPropertyName("exitCode")]
    public int ExitCode { get; set; }

    [JsonPropertyName("output")]
    public string Output { get; set; } = string.Empty;

    [JsonPropertyName("noop")]
    public bool WasNoOp { get; set; }

    [JsonPropertyName("elapsedMs")]
    public long ElapsedMs { get; set; }

    [JsonPropertyName("error")]
    public string? Error { get; set; }

    public static string Serialize(DaemonResponse response) =>
        JsonSerializer.Serialize(response);

    public static DaemonResponse? Deserialize(string json) =>
        JsonSerializer.Deserialize<DaemonResponse>(json);
}

public static class DaemonPaths
{
    public static string GetPipeName(string projectRoot)
    {
        var hash = IgniteBT.Core.Hashing.FastHash.HashString(Path.GetFullPath(projectRoot).ToLowerInvariant());
        return OperatingSystem.IsWindows()
            ? $"ignitebt-{hash[..16]}"
            : Path.Combine(Path.GetTempPath(), $"ignitebt-{hash[..16]}.sock");
    }

    public static string GetPidFilePath(string projectRoot) =>
        Path.Combine(projectRoot, "Build", "Temp", "ignitebt-daemon.pid");
}
