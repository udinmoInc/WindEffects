using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Serilog;
using IgniteBT.Build.Layout;
using IgniteBT.Build.Dependencies;

namespace IgniteBT.Workspace.Modules;

/// <summary>
/// Raised when one or more module Build.cs files fail to compile or load.
/// </summary>
public sealed class ModuleDiscoveryException : Exception
{
    public ModuleDiscoveryException(string message) : base(message)
    {
    }
}

/// <summary>
/// Discovers and loads module Build.cs files from the engine source tree.
/// </summary>
public class ModuleDiscoverer
{
    private readonly string _engineDirectory;
    private readonly string _configuration;
    private readonly string _platform;
    private readonly string _architecture;
    private readonly List<DiscoveredModule> _modules = new();
    private DependencyResolutionResult? _dependencyResult;

    public ModuleDiscoverer(
        string engineDirectory,
        string configuration,
        string platform,
        string architecture = "x64")
    {
        _engineDirectory = engineDirectory;
        _configuration = configuration;
        _platform = platform;
        _architecture = architecture;
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

        var buildCsFiles = Directory.GetFiles(_engineDirectory, "*.Build.cs", SearchOption.AllDirectories);
        Log.Information("Found {Count} Build.cs files", buildCsFiles.Length);

        foreach (var buildCsFile in buildCsFiles)
        {
            await LoadModuleAsync(buildCsFile);
        }

        Log.Information("Successfully discovered {Count} modules", _modules.Count);
        return _modules;
    }

    private async Task LoadModuleAsync(string buildCsPath)
    {
        Log.Information("Loading module: {Path}", buildCsPath);

        var buildCsContent = await File.ReadAllTextAsync(buildCsPath);
        var preparedSource = PrepareBuildCsSource(buildCsContent);
        var syntaxTree = CSharpSyntaxTree.ParseText(
            preparedSource,
            path: buildCsPath,
            encoding: System.Text.Encoding.UTF8);

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

        references.Add(MetadataReference.CreateFromFile(typeof(ModuleRules).Assembly.Location));

        var compilation = CSharpCompilation.Create(
            assemblyName: Path.GetFileNameWithoutExtension(buildCsPath) + "_compiled",
            syntaxTrees: new[] { syntaxTree },
            references: references,
            options: new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary)
                .WithOptimizationLevel(OptimizationLevel.Release));

        using var ms = new MemoryStream();
        var emitResult = compilation.Emit(ms);

        if (!emitResult.Success)
        {
            var errors = string.Join(
                Environment.NewLine,
                emitResult.Diagnostics
                    .Where(d => d.Severity == DiagnosticSeverity.Error)
                    .Select(d => d.ToString()));

            throw new ModuleDiscoveryException(
                $"Failed to compile Build.cs '{buildCsPath}':{Environment.NewLine}{errors}");
        }

        ms.Seek(0, SeekOrigin.Begin);
        var assembly = System.Runtime.Loader.AssemblyLoadContext.Default.LoadFromStream(ms);

        var moduleType = assembly.GetTypes()
            .FirstOrDefault(t => t.IsClass && !t.IsAbstract && typeof(ModuleRules).IsAssignableFrom(t));

        if (moduleType is null)
        {
            throw new ModuleDiscoveryException(
                $"No concrete ModuleRules subclass found in Build.cs '{buildCsPath}'.");
        }

        var context = new ModuleContext(_engineDirectory, _configuration, _platform, _architecture);

        if (_dependencyResult is not null)
        {
            var availableSDKs = new Dictionary<string, bool>();
            foreach (var (name, sdk) in _dependencyResult.SDKs)
            {
                availableSDKs[name] = sdk.IsValid;
            }
            context.SetAvailableSDKs(availableSDKs);

            var availableThirdParty = new Dictionary<string, bool>();
            foreach (var (name, lib) in _dependencyResult.ThirdPartyLibraries)
            {
                availableThirdParty[name] = lib.IsValid;
            }
            context.SetAvailableThirdParty(availableThirdParty);
            context.SetFeatureFlags(_dependencyResult.FeatureFlags);
        }

        if (Activator.CreateInstance(moduleType, context) is not ModuleRules moduleInstance)
        {
            throw new ModuleDiscoveryException(
                $"Failed to instantiate module class '{moduleType.FullName}' from '{buildCsPath}'.");
        }

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

        if (discoveredModule.IsDisabled)
        {
            Log.Information("Module {Name} is disabled, skipping", moduleName);
            return;
        }

        if (_dependencyResult is not null)
        {
            foreach (var requiredSDK in discoveredModule.RequiredSDKs)
            {
                if (!_dependencyResult.SDKs.TryGetValue(requiredSDK, out var sdk) || !sdk.IsValid)
                {
                    throw new ModuleDiscoveryException(
                        $"Module '{moduleName}' requires SDK '{requiredSDK}' which is not available.");
                }
            }
        }

        _modules.Add(discoveredModule);
        Log.Information(
            "Loaded module: {Name} with {DepCount} dependencies",
            moduleName,
            discoveredModule.PublicDependencies.Count + discoveredModule.PrivateDependencies.Count);
    }

    private static string PrepareBuildCsSource(string content) =>
        content.Replace("using IgniteBT.BuildSystem;", "using IgniteBT.Build.Layout;", StringComparison.Ordinal);

    private static string ExtractModuleNameFromPath(string buildCsPath)
    {
        var fileName = Path.GetFileNameWithoutExtension(buildCsPath);
        return fileName.EndsWith(".Build", StringComparison.Ordinal) ? fileName[..^6] : fileName;
    }
}

/// <summary>
/// Represents a discovered module from the file system.
/// </summary>
public class DiscoveredModule
{
    public string Name { get; set; } = string.Empty;
    public string BuildCsPath { get; set; } = string.Empty;
    public string ModuleDirectory { get; set; } = string.Empty;
    public ModuleType Type { get; set; }
    public List<string> PublicDependencies { get; set; } = new();
    public List<string> PrivateDependencies { get; set; } = new();
    public List<string> PublicIncludePaths { get; set; } = new();
    public List<string> PrivateIncludePaths { get; set; } = new();
    public List<string> SourceFiles { get; set; } = new();
    public List<string> RequiredSDKs { get; set; } = new();
    public List<string> OptionalSDKs { get; set; } = new();
    public List<string> RequiredThirdParty { get; set; } = new();
    public List<string> OptionalThirdParty { get; set; } = new();
    public bool IsDisabled { get; set; }
    public List<string> Definitions { get; set; } = new();
}
