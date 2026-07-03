using System.Text.RegularExpressions;
using Serilog;

namespace IgniteBT.Dependency;

/// <summary>
/// Include directive information.
/// </summary>
public class IncludeDirective
{
    /// <summary>
    /// Included file path (as written in source).
    /// </summary>
    public string IncludedFile { get; set; } = string.Empty;

    /// <summary>
    /// Include type (angle brackets or quotes).
    /// </summary>
    public IncludeType Type { get; set; }

    /// <summary>
    /// Line number where the include appears.
    /// </summary>
    public int LineNumber { get; set; }
}

/// <summary>
/// Include directive type.
/// </summary>
public enum IncludeType
{
    /// <summary>
    /// System include: #include <file>
    /// </summary>
    System,

    /// <summary>
    /// Local include: #include "file"
    /// </summary>
    Local
}

/// <summary>
/// Dependency information for a source file.
/// </summary>
public class FileDependencies
{
    /// <summary>
    /// Source file path.
    /// </summary>
    public string SourceFile { get; set; } = string.Empty;

    /// <summary>
    /// All included headers (transitive closure).
    /// </summary>
    public List<string> IncludedHeaders { get; set; } = new();

    /// <summary>
    /// Direct includes only (non-transitive).
    /// </summary>
    public List<IncludeDirective> DirectIncludes { get; set; } = new();

    /// <summary>
    /// Hash of all dependencies.
    /// </summary>
    public string DependenciesHash { get; set; } = string.Empty;
}

/// <summary>
/// C/C++ dependency scanner for #include directives.
/// </summary>
public class DependencyScanner
{
    private readonly Dictionary<string, FileDependencies> _dependencyCache = new();
    private readonly List<string> _includePaths = new();

    /// <summary>
    /// Adds an include path for resolving local includes.
    /// </summary>
    public void AddIncludePath(string path)
    {
        if (!_includePaths.Contains(path))
        {
            _includePaths.Add(path);
        }
    }

    /// <summary>
    /// Scans a source file for dependencies.
    /// </summary>
    public async Task<FileDependencies> ScanFileAsync(string sourceFile, bool transitive = true)
    {
        if (_dependencyCache.TryGetValue(sourceFile, out var cached))
        {
            return cached;
        }

        Log.Information("Scanning dependencies for {SourceFile}", sourceFile);

        var dependencies = new FileDependencies
        {
            SourceFile = sourceFile
        };

        if (!File.Exists(sourceFile))
        {
            Log.Warning("Source file does not exist: {SourceFile}", sourceFile);
            return dependencies;
        }

        // Parse direct includes
        dependencies.DirectIncludes = ParseIncludes(sourceFile);

        if (transitive)
        {
            // Resolve transitive dependencies
            var visited = new HashSet<string>();
            dependencies.IncludedHeaders = await ResolveTransitiveDependencies(sourceFile, visited);
        }
        else
        {
            // Only resolve direct includes to actual file paths
            dependencies.IncludedHeaders = dependencies.DirectIncludes
                .Select(inc => ResolveIncludePath(inc.IncludedFile, inc.Type, Path.GetDirectoryName(sourceFile)))
                .Where(path => !string.IsNullOrEmpty(path))
                .Distinct()
                .ToList()!;
        }

        // Compute hash of all dependencies
        dependencies.DependenciesHash = ComputeDependenciesHash(dependencies.IncludedHeaders);

        _dependencyCache[sourceFile] = dependencies;

        Log.Information("Found {Count} dependencies for {SourceFile}", dependencies.IncludedHeaders.Count, sourceFile);

        return dependencies;
    }

    /// <summary>
    /// Parses #include directives from a source file.
    /// </summary>
    private List<IncludeDirective> ParseIncludes(string filePath)
    {
        var includes = new List<IncludeDirective>();
        var lines = File.ReadAllLines(filePath);

        for (int i = 0; i < lines.Length; i++)
        {
            var line = lines[i].Trim();
            
            // Skip preprocessor directives that aren't includes
            if (!line.StartsWith("#include"))
            {
                continue;
            }

            // Match #include "file" or #include <file>
            var match = Regex.Match(line, @"#include\s+[""<]([^>""]+)[>""]");
            if (match.Success)
            {
                var includedFile = match.Groups[1].Value;
                var type = line.Contains('<') ? IncludeType.System : IncludeType.Local;

                includes.Add(new IncludeDirective
                {
                    IncludedFile = includedFile,
                    Type = type,
                    LineNumber = i + 1
                });
            }
        }

        return includes;
    }

    /// <summary>
    /// Resolves transitive dependencies recursively.
    /// </summary>
    private async Task<List<string>> ResolveTransitiveDependencies(string sourceFile, HashSet<string> visited)
    {
        var dependencies = new List<string>();

        if (visited.Contains(sourceFile))
        {
            return dependencies; // Avoid circular dependencies
        }

        visited.Add(sourceFile);

        var directIncludes = ParseIncludes(sourceFile);
        var sourceDir = Path.GetDirectoryName(sourceFile);

        foreach (var include in directIncludes)
        {
            var resolvedPath = ResolveIncludePath(include.IncludedFile, include.Type, sourceDir);
            
            if (!string.IsNullOrEmpty(resolvedPath) && File.Exists(resolvedPath))
            {
                dependencies.Add(resolvedPath);

                // Recursively scan header files
                if (resolvedPath.EndsWith(".h") || resolvedPath.EndsWith(".hpp") || resolvedPath.EndsWith(".hxx"))
                {
                    var transitiveDeps = await ResolveTransitiveDependencies(resolvedPath, visited);
                    dependencies.AddRange(transitiveDeps);
                }
            }
        }

        return dependencies.Distinct().ToList();
    }

    /// <summary>
    /// Resolves an include path to an actual file path.
    /// </summary>
    private string? ResolveIncludePath(string includedFile, IncludeType type, string? sourceDirectory)
    {
        // Try relative to source directory first for local includes
        if (type == IncludeType.Local && !string.IsNullOrEmpty(sourceDirectory))
        {
            var relativePath = Path.Combine(sourceDirectory, includedFile);
            if (File.Exists(relativePath))
            {
                return Path.GetFullPath(relativePath);
            }
        }

        // Try include paths
        foreach (var includePath in _includePaths)
        {
            var fullPath = Path.Combine(includePath, includedFile);
            if (File.Exists(fullPath))
            {
                return Path.GetFullPath(fullPath);
            }
        }

        // For system includes, try common system paths
        if (type == IncludeType.System)
        {
            // This is platform-specific and would need to be expanded
            // For now, just log that we couldn't resolve it
            Log.Debug("Could not resolve system include: {Include}", includedFile);
        }

        return null;
    }

    /// <summary>
    /// Computes a hash of all dependency file paths.
    /// </summary>
    private string ComputeDependenciesHash(List<string> dependencies)
    {
        using var sha256 = System.Security.Cryptography.SHA256.Create();
        
        var sortedDeps = dependencies.OrderBy(d => d).ToList();
        foreach (var dep in sortedDeps)
        {
            var bytes = System.Text.Encoding.UTF8.GetBytes(dep);
            sha256.TransformBlock(bytes, 0, bytes.Length, null, 0);
        }
        
        sha256.TransformFinalBlock(Array.Empty<byte>(), 0, 0);
        return Convert.ToHexString(sha256.Hash ?? Array.Empty<byte>()).ToLowerInvariant();
    }

    /// <summary>
    /// Clears the dependency cache.
    /// </summary>
    public void ClearCache()
    {
        _dependencyCache.Clear();
    }

    /// <summary>
    /// Invalidates cached dependencies for a specific file.
    /// </summary>
    public void Invalidate(string sourceFile)
    {
        _dependencyCache.Remove(sourceFile);
    }

    /// <summary>
    /// Gets cached dependencies for a file if available.
    /// </summary>
    public bool TryGetCachedDependencies(string sourceFile, out FileDependencies? dependencies)
    {
        return _dependencyCache.TryGetValue(sourceFile, out dependencies);
    }
}
