// ==============================================================================
// WindEffects — IgniteBT — IRemoteExecutor
// Source file for the IgniteBT module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
namespace IgniteBT.Distributed;

/// <summary>
/// Executes build actions on remote workers with compression and retry.
/// </summary>
public interface IRemoteExecutor
{
    Task<RemoteExecutionResult> ExecuteAsync(RemoteExecutionRequest request, CancellationToken cancellationToken =
        default);
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
