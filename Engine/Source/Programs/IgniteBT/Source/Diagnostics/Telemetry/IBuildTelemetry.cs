// ==============================================================================
// WindEffects — IgniteBT — IBuildTelemetry
// Decoupled build telemetry contract for clean runtime vs debug modes.
// Maintained and authored by Vijay Singh and John Anderson.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
namespace IgniteBT.Diagnostics.Telemetry;

/// <summary>
/// Minimal, lightweight telemetry interface separating core build pipeline execution
/// from profiling, benchmarking, and console logging.
/// </summary>
public interface IBuildTelemetry
{
    void OnBuildStarted(string targetName, int totalTus);
    void OnBuildFinished(bool success, double totalMs, bool wasNoOp);
    void OnCompilationStarted(string fileBasename, int current, int total);
    void OnCompilationFinished(string fileBasename, bool success, double elapsedMs);
    void OnLinkingStarted(string targetName);
    void OnLinkingFinished(string targetName, bool success, double elapsedMs);
    void OnDiagnosticEvent(string category, string message);
}
