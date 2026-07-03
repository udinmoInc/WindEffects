using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Serilog;
using IgniteBT.Build.Layout;
using IgniteBT.Build.Dependencies;

namespace IgniteBT.Workspace.Modules;

/// <summary>
/// Discovers and loads module Build.cs files from the engine source tree.
/// </summary>
public class ModuleDiscoverer
{
    private readonly string _engineDirectory;
    private readonly List<DiscoveredModule> _modules = new();
    private DependencyResolutionResult? _dependencyResult;

    public ModuleDiscoverer(string engineDirectory)
    {
        _engineDirectory = engineDirectory;
    }

    /// <summary>
    /// Sets the dependency resolution result.
    /// </summary>
    public void SetDependencyResult(DependencyResolutionResult result)
    {
        _dependencyResult = result;
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
            
            // Build reference list from TRUSTED_PLATFORM_ASSEMBLIES to include all .NET facade assemblies
            var trustedAssemblies = AppContext.GetData("TRUSTED_PLATFORM_ASSEMBLIES") as string;
            var references = new List<MetadataReference>();
            
            if (!string.IsNullOrEmpty(trustedAssemblies))
            {
                foreach (var assemblyPath in trustedAssemblies.Split(Path.PathSeparator))
                {
                    if (!string.IsNullOrEmpty(assemblyPath) && File.Exists(assemblyPath))
                    {
                        references.Add(MetadataReference.CreateFromFile(assemblyPath));
                    }
                }
            }
            
            // Also explicitly add the ModuleRules assembly
            references.Add(MetadataReference.CreateFromFile(typeof(ModuleRules).Assembly.Location));

            var compilation = CSharpCompilation.Create(
                assemblyName: Path.GetFileNameWithoutExtension(buildCsPath) + "_compiled",
                syntaxTrees: new[] { syntaxTree },
                references: references,
                options: new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary).WithOptimizationLevel(OptimizationLevel.Release)
            );

            using var ms = new System.IO.MemoryStream();
            var emitResult = compilation.Emit(ms);

            if (!emitResult.Success)
            {
                var errors = string.Join("\n", emitResult.Diagnostics.Where(d => d.Severity == Microsoft.CodeAnalysis.DiagnosticSeverity.Error));
                Log.Warning("Failed to compile Build.cs file: {Errors}", errors);
                
                // Fall back to simple parsing
                LoadModuleSimple(buildCsPath, buildCsContent);
                return;
            }

            ms.Seek(0, System.IO.SeekOrigin.Begin);
            var assembly = System.Runtime.Loader.AssemblyLoadContext.Default.LoadFromStream(ms);

            // Find the module class (should inherit from ModuleRules)
            var moduleType = assembly.GetTypes()
                .FirstOrDefault(t => t.IsClass && !t.IsAbstract && typeof(ModuleRules).IsAssignableFrom(t));

            if (moduleType == null)
            {
                Log.Warning("No class inheriting from ModuleRules found in {Path}", buildCsPath);
                LoadModuleSimple(buildCsPath, buildCsContent);
                return;
            }

            // Create module context with dependency information
            var context = new ModuleContext(_engineDirectory, "Release", "Windows", "x64");
            
            if (_dependencyResult != null)
            {
                // Set available SDKs
                var availableSDKs = new Dictionary<string, bool>();
                foreach (var (name, sdk) in _dependencyResult.SDKs)
                {
                    availableSDKs[name] = sdk.IsValid;
                }
                context.SetAvailableSDKs(availableSDKs);
                
                // Set available third-party libraries
                var availableThirdParty = new Dictionary<string, bool>();
                foreach (var (name, lib) in _dependencyResult.ThirdPartyLibraries)
                {
                    availableThirdParty[name] = lib.IsValid;
                }
                context.SetAvailableThirdParty(availableThirdParty);
                
                // Set feature flags
                context.SetFeatureFlags(_dependencyResult.FeatureFlags);
            }
            
            // Instantiate the module
            var moduleInstance = (ModuleRules?)Activator.CreateInstance(moduleType, context);
            
            if (moduleInstance == null)
            {
                Log.Warning("Failed to instantiate module class from {Path}", buildCsPath);
                LoadModuleSimple(buildCsPath, buildCsContent);
                return;
            }

            // Extract module information
            var moduleName = ExtractModuleNameFromPath(buildCsPath);
            var moduleDirectory = Path.GetDirectoryName(buildCsPath) ?? string.Empty;

            var discoveredModule = new DiscoveredModule
            {
                Name = moduleName,
                BuildCsPath = buildCsPath,
                ModuleDirectory = moduleDirectory,
                Type = moduleInstance.Type,
                PublicDependencies = new List<string>(moduleInstance.PublicDependencies),
                PrivateDependencies = new List<string>(moduleInstance.PrivateDependencies),
                PublicIncludePaths = new List<string>(moduleInstance.PublicIncludePaths),
                PrivateIncludePaths = new List<string>(moduleInstance.PrivateIncludePaths),
                SourceFiles = new List<string>(moduleInstance.SourceFiles),
                RequiredSDKs = new List<string>(moduleInstance.RequiredSDKs),
                OptionalSDKs = new List<string>(moduleInstance.OptionalSDKs),
                RequiredThirdParty = new List<string>(moduleInstance.RequiredThirdParty),
                OptionalThirdParty = new List<string>(moduleInstance.OptionalThirdParty),
                IsDisabled = moduleInstance.IsDisabled,
                Definitions = new List<string>(moduleInstance.Definitions)
            };

            // Skip disabled modules
            if (discoveredModule.IsDisabled)
            {
                Log.Information("Module {Name} is disabled, skipping", moduleName);
                return;
            }

            // Check required SDKs
            if (_dependencyResult != null)
            {
                foreach (var requiredSDK in discoveredModule.RequiredSDKs)
                {
                    if (!_dependencyResult.SDKs.TryGetValue(requiredSDK, out var sdk) || !sdk.IsValid)
                    {
                        Log.Error("Module {Name} requires SDK {SDK} which is not available", moduleName, requiredSDK);
                        discoveredModule.IsDisabled = true;
                        return;
                    }
                }
            }

            _modules.Add(discoveredModule);
            Log.Information("Loaded module: {Name} with {DepCount} dependencies", moduleName, 
                discoveredModule.PublicDependencies.Count + discoveredModule.PrivateDependencies.Count);
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Failed to load module from {Path}", buildCsPath);
        }
    }

    /// <summary>
    /// Simple fallback loading that parses dependencies from the Build.cs file text.
    /// </summary>
    private void LoadModuleSimple(string buildCsPath, string buildCsContent)
    {
        var moduleName = ExtractModuleNameFromPath(buildCsPath);
        var moduleDirectory = Path.GetDirectoryName(buildCsPath) ?? string.Empty;

        var discoveredModule = new DiscoveredModule
        {
            Name = moduleName,
            BuildCsPath = buildCsPath,
            ModuleDirectory = moduleDirectory,
            Type = ModuleType.SharedLibrary
        };

        // Simple regex-based parsing of Dependencies.Add() calls
        var depMatches = System.Text.RegularExpressions.Regex.Matches(buildCsContent, @"Dependencies\.Add\(""([^""]+)""\)");
        foreach (System.Text.RegularExpressions.Match match in depMatches)
        {
            discoveredModule.PublicDependencies.Add(match.Groups[1].Value);
        }

        _modules.Add(discoveredModule);
        Log.Information("Loaded module (simple): {Name} with {DepCount} dependencies", moduleName, 
            discoveredModule.PublicDependencies.Count);
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

    /// <summary>
    /// Required SDKs.
    /// </summary>
    public List<string> RequiredSDKs { get; set; } = new();

    /// <summary>
    /// Optional SDKs.
    /// </summary>
    public List<string> OptionalSDKs { get; set; } = new();

    /// <summary>
    /// Required third-party libraries.
    /// </summary>
    public List<string> RequiredThirdParty { get; set; } = new();

    /// <summary>
    /// Optional third-party libraries.
    /// </summary>
    public List<string> OptionalThirdParty { get; set; } = new();

    /// <summary>
    /// Whether this module is disabled.
    /// </summary>
    public bool IsDisabled { get; set; }

    /// <summary>
    /// Compiler definitions.
    /// </summary>
    public List<string> Definitions { get; set; } = new();
}
