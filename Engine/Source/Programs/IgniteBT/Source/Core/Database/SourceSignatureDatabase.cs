// ==============================================================================
// WindEffects — IgniteBT — SourceSignatureDatabase
// Source file for the IgniteBT module.
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.Core.Hashing;

namespace IgniteBT.Core.Database;

public sealed class SourceSignatureRecord
{
    public string FilePath { get; init; } = string.Empty;
    public string ContentHash { get; init; } = string.Empty;
    public string SymbolHash { get; init; } = string.Empty;
    public string IncludeHash { get; init; } = string.Empty;
    public string FunctionsSummary { get; init; } = string.Empty;
    public string ClassesSummary { get; init; } = string.Empty;
    public string CompilerIdentity { get; init; } = string.Empty;
    public string PchIdentity { get; init; } = string.Empty;
    public string ObjectIdentity { get; init; } = string.Empty;
    public string LastClassifiedUtc { get; init; } = DateTime.UtcNow.ToString("o");
}

public static class SourceSignatureDatabase
{
    public static SourceSignatureRecord ComputeSignature(
        string filePath,
        string content,
        string compilerVersion,
        string pchHeader = "")
    {
        var contentHash = FastHash.HashString(content);
        var lines = content.Split('\n');
        var includes = lines.Where(l => l.TrimStart().StartsWith("#include")).ToList();
        var includeHash = FastHash.HashString(string.Join("|", includes));

        var symbolLines = lines.Where(l =>
            l.Contains("class ") || l.Contains("struct ") || l.Contains("enum ") || l.Contains("#define") || l.Contains("template")).ToList();
        var symbolHash = FastHash.HashString(string.Join("|", symbolLines));

        return new SourceSignatureRecord
        {
            FilePath = filePath,
            ContentHash = contentHash,
            SymbolHash = symbolHash,
            IncludeHash = includeHash,
            FunctionsSummary = $"{lines.Length} lines, {symbolLines.Count} symbol decls",
            ClassesSummary = $"{includes.Count} includes",
            CompilerIdentity = compilerVersion,
            PchIdentity = pchHeader,
            ObjectIdentity = FastHash.HashString($"{filePath}|{contentHash}|{compilerVersion}"),
            LastClassifiedUtc = DateTime.UtcNow.ToString("o")
        };
    }
}
