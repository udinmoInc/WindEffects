namespace IgniteBT.Distributed;

/// <summary>
/// Remote compiler interface — future-ready for distributed build farms.
/// </summary>
public interface IRemoteCompiler
{
    Task<RemoteCompileResult> CompileAsync(RemoteCompileRequest request, CancellationToken cancellationToken = default);
    Task<bool> HealthCheckAsync(CancellationToken cancellationToken = default);
    string WorkerId { get; }
    string HostAddress { get; }
}

public sealed class RemoteCompileRequest
{
    public string SourceFile { get; set; } = string.Empty;
    public string OutputFile { get; set; } = string.Empty;
    public string CommandLine { get; set; } = string.Empty;
    public string CompilerHash { get; set; } = string.Empty;
    public byte[]? SourceSnapshot { get; set; }
    public List<string> HeaderHashes { get; set; } = new();
}

public sealed class RemoteCompileResult
{
    public bool Success { get; set; }
    public byte[]? ObjectData { get; set; }
    public string StandardError { get; set; } = string.Empty;
    public long CompileTimeMs { get; set; }
}
