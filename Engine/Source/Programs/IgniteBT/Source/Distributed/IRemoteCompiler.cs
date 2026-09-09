// ==============================================================================
// WindEffects — IgniteBT — IRemoteCompiler
// Source file for the IgniteBT module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
