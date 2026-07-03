using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Serilog;
using IgniteBT.BuildSystem;

namespace IgniteBT.ModuleDiscovery;

/// <summary>
/// Discovers and loads module Build.cs files from the engine source tree.
/// </summary>
public class ModuleDiscoverer
{
    private readonly string _engineDirectory;
    private readonly List<DiscoveredModule> _modules = new();

    public ModuleDiscoverer(string engineDirectory)
    {
        _engineDirectory = engineDirectory;
    }

    /// <summary>
    /// Discovers all modules in the engine directory.
    /// </summary>
    public async Task<List<DiscoveredModule>> DiscoverModulesAsync()
    {
        Log.Information("Discovering modules in {EngineDirectory}", _engineDirectory);
        _modules.Clear();

        // Search for *.Build.cs files
        var buildCsFiles = Directory.GetFiles(_engineDirectory, "*.Build.cs", SearchOption.AllDirectories);

        Log.Information("Found {Count} Build.cs files", buildCsFiles.Length);

        foreach (var buildCsFile in buildCsFiles)
        {
            await LoadModuleAsync(buildCsFile);
        }

        Log.Information("Successfully discovered {Count} modules", _modules.Count);
        return _modules;
    }

    /// <summary>
    /// Loads a single module from its Build.cs file.
    /// </summary>
    private async Task LoadModuleAsync(string buildCsPath)
    {
        try
        {
            Log.Information("Loading module: {Path}", buildCsPath);

            // Read the Build.cs file
            var buildCsContent = await File.ReadAllTextAsync(buildCsPath);

            // Compile the Build.cs file using Roslyn
            var syntaxTree = CSharpSyntaxTree.ParseText(buildCsContent);
            
            var references = new[]
            {
                MetadataReference.CreateFromFile(typeof(object).Assembly.Location),
                MetadataReference.CreateFromFile(typeof(ModuleRules).Assembly.Location)
            };

            var compilation = CSharpCompilation.Create(
                assemblyName: Path.GetFileNameWithoutExtension(buildCsPath),
                syntaxTrees: new[] { syntaxTree },
                references: references,
                options: new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary)
            );

            // TODO: Actually compile and instantiate the module rules
            // For now, just create a placeholder discovered module
            var moduleName = ExtractModuleNameFromPath(buildCsPath);
            var moduleDirectory = Path.GetDirectoryName(buildCsPath) ?? string.Empty;

            var discoveredModule = new DiscoveredModule
            {
                Name = moduleName,
                BuildCsPath = buildCsPath,
                ModuleDirectory = moduleDirectory,
                Type = ModuleType.SharedLibrary // Default
            };

            _modules.Add(discoveredModule);
            Log.Information("Loaded module: {Name}", moduleName);
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Failed to load module from {Path}", buildCsPath);
        }
    }

    /// <summary>
    /// Extracts the module name from the Build.cs file path.
    /// </summary>
    private string ExtractModuleNameFromPath(string buildCsPath)
    {
        var fileName = Path.GetFileNameWithoutExtension(buildCsPath);
        // Remove ".Build" suffix if present
        return fileName.EndsWith(".Build") ? fileName[..^6] : fileName;
    }
}

/// <summary>
/// Represents a discovered module from the file system.
/// </summary>
public class DiscoveredModule
{
    /// <summary>
    /// Module name.
    /// </summary>
    public string Name { get; set; } = string.Empty;

    /// <summary>
    /// Path to the Build.cs file.
    /// </summary>
    public string BuildCsPath { get; set; } = string.Empty;

    /// <summary>
    /// Module directory path.
    /// </summary>
    public string ModuleDirectory { get; set; } = string.Empty;

    /// <summary>
    /// Module type.
    /// </summary>
    public ModuleType Type { get; set; }

    /// <summary>
    /// Public dependencies.
    /// </summary>
    public List<string> PublicDependencies { get; set; } = new();

    /// <summary>
    /// Private dependencies.
    /// </summary>
    public List<string> PrivateDependencies { get; set; } = new();

    /// <summary>
    /// Public include paths.
    /// </summary>
    public List<string> PublicIncludePaths { get; set; } = new();

    /// <summary>
    /// Private include paths.
    /// </summary>
    public List<string> PrivateIncludePaths { get; set; } = new();

    /// <summary>
    /// Source files.
    /// </summary>
    public List<string> SourceFiles { get; set; } = new();
}
