namespace IgniteBT.Distributed;

/// <summary>
/// Executes build actions on remote workers with compression and retry.
/// </summary>
public interface IRemoteExecutor
{
    Task<RemoteExecutionResult> ExecuteAsync(RemoteExecutionRequest request, CancellationToken cancellationToken = default);
    bool IsEnabled { get; }
}

public sealed class RemoteExecutionRequest
{
    public string ActionType { get; set; } = string.Empty;
    public string PayloadJson { get; set; } = string.Empty;
    public string PreferredWorkerId { get; set; } = string.Empty;
    public bool CompressPayload { get; set; } = true;
}

public sealed class RemoteExecutionResult
{
    public bool Success { get; set; }
    public string ResponseJson { get; set; } = string.Empty;
    public string WorkerId { get; set; } = string.Empty;
    public long RoundTripMs { get; set; }
}
