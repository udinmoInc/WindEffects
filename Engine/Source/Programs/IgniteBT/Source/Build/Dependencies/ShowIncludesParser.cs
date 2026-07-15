using System.Text.RegularExpressions;
using Serilog;

namespace IgniteBT.Build.Dependencies;

/// <summary>
/// Parses compiler-generated dependency output from MSVC /showIncludes, clang -MD, and gcc -MMD.
/// Produces exact include graphs — no regex scanning of source files.
/// </summary>
public static class ShowIncludesParser
{
    private static readonly Regex MsvcIncludeRegex = new(
        @"^Note: including file:\s+(.+)$",
        RegexOptions.Compiled | RegexOptions.IgnoreCase);

    /// <summary>
    /// Parse MSVC /showIncludes output (written to stderr).
    /// </summary>
    public static List<string> ParseMsvcShowIncludes(string stderr)
    {
        var headers = new List<string>();
        foreach (var line in stderr.Split('\n', StringSplitOptions.RemoveEmptyEntries))
        {
            var trimmed = line.TrimEnd('\r');
            var match = MsvcIncludeRegex.Match(trimmed);
            if (match.Success)
            {
                var path = match.Groups[1].Value.Trim();
                if (!string.IsNullOrEmpty(path))
                    headers.Add(NormalizePath(path));
            }
        }
        return headers.Distinct(StringComparer.OrdinalIgnoreCase).ToList();
    }

    /// <summary>
    /// Parse clang/gcc -MD makefile dependency output.
    /// Format: source.obj: header1.h header2.h ...
    /// </summary>
    public static List<string> ParseMakefileDependencies(string depFileContent, string sourceFile)
    {
        var headers = new List<string>();
        if (string.IsNullOrWhiteSpace(depFileContent)) return headers;

        // Strip continuation lines (backslash-newline)
        var normalized = depFileContent.Replace("\\\r\n", " ").Replace("\\\n", " ");

        var colonIndex = normalized.IndexOf(':');
        if (colonIndex < 0) return headers;

        var depsPart = normalized[(colonIndex + 1)..];
        foreach (var token in depsPart.Split([' ', '\t'], StringSplitOptions.RemoveEmptyEntries))
        {
            var path = token.Trim();
            if (path.Length == 0) continue;
            if (path.Equals(sourceFile, StringComparison.OrdinalIgnoreCase)) continue;
            if (path.EndsWith(".obj", StringComparison.OrdinalIgnoreCase)) continue;
            headers.Add(NormalizePath(path));
        }

        return headers.Distinct(StringComparer.OrdinalIgnoreCase).ToList();
    }

    /// <summary>
    /// Parse dependency file from disk (clang/gcc -MF output).
    /// </summary>
    public static List<string> ParseDependencyFile(string depFilePath, string sourceFile)
    {
        if (!File.Exists(depFilePath)) return new List<string>();
        try
        {
            return ParseMakefileDependencies(File.ReadAllText(depFilePath), sourceFile);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to parse dependency file {Path}", depFilePath);
            return new List<string>();
        }
    }

    private static string NormalizePath(string path)
    {
        try { return Path.GetFullPath(path); }
        catch { return path; }
    }
}
