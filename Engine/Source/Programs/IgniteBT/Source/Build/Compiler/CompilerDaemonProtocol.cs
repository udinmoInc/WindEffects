using System.Text.Json;

namespace IgniteBT.Build.Compiler;

/// <summary>
/// Line-delimited JSON protocol for persistent compiler worker IPC.
/// </summary>
public static class CompilerDaemonProtocol
{
    public const string CompileCommand = "compile";
    public const string PingCommand = "ping";
    public const string ShutdownCommand = "shutdown";

    public static string SerializeRequest(CompilerDaemonRequest request) =>
        JsonSerializer.Serialize(request);

    public static CompilerDaemonRequest? DeserializeRequest(string line) =>
        JsonSerializer.Deserialize<CompilerDaemonRequest>(line);

    public static string SerializeResponse(CompilerDaemonResponse response) =>
        JsonSerializer.Serialize(response);

    public static CompilerDaemonResponse? DeserializeResponse(string line) =>
        JsonSerializer.Deserialize<CompilerDaemonResponse>(line);
}

public sealed class CompilerDaemonRequest
{
    public string Command { get; set; } = CompilerDaemonProtocol.CompileCommand;
    public string RequestId { get; set; } = Guid.NewGuid().ToString("N");
    public string CompilerExecutable { get; set; } = string.Empty;
    public string Arguments { get; set; } = string.Empty;
    public string WorkingDirectory { get; set; } = string.Empty;
    public bool CaptureDependencies { get; set; }
    public string DependencyOutputPath { get; set; } = string.Empty;
}

public sealed class CompilerDaemonResponse
{
    public string RequestId { get; set; } = string.Empty;
    public bool Success { get; set; }
    public int ExitCode { get; set; }
    public string StandardOutput { get; set; } = string.Empty;
    public string StandardError { get; set; } = string.Empty;
    public long ElapsedMs { get; set; }
    public string? ErrorMessage { get; set; }
}
