// ==============================================================================
// WindEffects — IgniteBT — NullBuildTelemetry
// Zero-overhead no-op telemetry provider.
// Maintained and authored by Vijay Singh and John Anderson.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
namespace IgniteBT.Diagnostics.Telemetry;

/// <summary>
/// No-op implementation of IBuildTelemetry used in headless/silent builds.
/// </summary>
public sealed class NullBuildTelemetry : IBuildTelemetry
{
    public static readonly NullBuildTelemetry Instance = new();

    public void OnBuildStarted(string targetName, int totalTus) { }
    public void OnBuildFinished(bool success, double totalMs, bool wasNoOp) { }
    public void OnCompilationStarted(string fileBasename, int current, int total) { }
    public void OnCompilationFinished(string fileBasename, bool success, double elapsedMs) { }
    public void OnLinkingStarted(string targetName) { }
    public void OnLinkingFinished(string targetName, bool success, double elapsedMs) { }
    public void OnDiagnosticEvent(string category, string message) { }
}
