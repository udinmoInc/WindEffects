using Serilog;
using IgniteBT.Workspace.SDK;
using IgniteBT.Workspace.ThirdParty;

namespace IgniteBT.Build.Dependencies;

/// <summary>
/// Resolves dependencies including SDKs, third-party libraries, and modules.
/// </summary>
public class DependencyResolver
{
    private readonly SDKManager _sdkManager;
    private readonly ThirdPartyManager _thirdPartyManager;
    
    private readonly Dictionary<string, Dependency> _dependencies = new();
    private readonly Dictionary<string, string> _featureFlags = new();
    private readonly List<string> _warnings = new();
    private readonly List<string> _errors = new();
    
    public DependencyResolver()
    {
        _sdkManager = SDKManager.Instance;
        _thirdPartyManager = ThirdPartyManager.Instance;
    }
    
    /// <summary>
    /// Resolves all dependencies for the build.
    /// </summary>
    public async Task<DependencyResolutionResult> ResolveAsync(string? compilerPath = null)
    {
        Log.Information("Resolving dependencies...");
        
        // Initialize managers
        _sdkManager.Initialize();
        _thirdPartyManager.Initialize();
        
        // Detect all SDKs (fast path when cache fingerprint is valid)
        var detectedSDKs = await _sdkManager.DetectAllAsync(compilerPath);
        
        // Get all third-party libraries
        var thirdPartyLibs = _thirdPartyManager.GetAllLibraries();
        
        // Generate feature flags
        GenerateFeatureFlags(detectedSDKs, thirdPartyLibs);
        
        // Build dependency graph
        BuildDependencyGraph(detectedSDKs, thirdPartyLibs);
        
        // Validate dependencies
        ValidateDependencies();
        
        var result = new DependencyResolutionResult
        {
            SDKs = detectedSDKs,
            ThirdPartyLibraries = thirdPartyLibs,
            FeatureFlags = _featureFlags,
            Dependencies = _dependencies,
            Warnings = _warnings,
            Errors = _errors,
            Success = _errors.Count == 0
        };
        
        Log.Information("Dependency resolution complete: {Success}, {SDKCount} SDKs, {LibCount} libraries, {FlagCount} feature flags",
            result.Success, detectedSDKs.Count, thirdPartyLibs.Count, _featureFlags.Count);
        
        return result;
    }
    
    /// <summary>
    /// Generates feature flags based on available SDKs and libraries.
    /// </summary>
    private void GenerateFeatureFlags(Dictionary<string, SDKInfo> sdks, Dictionary<string, ThirdPartyLibrary> libraries)
    {
        _featureFlags.Clear();
        
        // Generate SDK feature flags
        foreach (var (name, sdk) in sdks)
        {
            if (sdk.IsValid)
            {
                var flagName = $"WE_HAS_{name.ToUpperInvariant().Replace(" ", "_")}";
                _featureFlags[flagName] = "1";
                Log.Debug("Feature flag: {Flag}=1", flagName);
            }
        }

        // Canonical Vulkan availability flag used by C++ #if WE_HAS_VULKAN guards
        if (sdks.TryGetValue("VulkanSDK", out var vulkanSdk) && vulkanSdk.IsValid)
        {
            _featureFlags["WE_HAS_VULKAN"] = "1";
            Log.Debug("Feature flag: WE_HAS_VULKAN=1");
        }
        
        // Generate third-party library feature flags
        foreach (var (name, lib) in libraries)
        {
            if (lib.IsValid)
            {
                var flagName = $"WE_HAS_{name.ToUpperInvariant().Replace("-", "_").Replace(".", "_")}";
                _featureFlags[flagName] = "1";
                Log.Debug("Feature flag: {Flag}=1", flagName);
            }
        }
        
        // Platform-specific flags
        if (System.Runtime.InteropServices.RuntimeInformation.IsOSPlatform(
            System.Runtime.InteropServices.OSPlatform.Windows))
        {
            _featureFlags["WE_PLATFORM_WINDOWS"] = "1";
        }
        else if (System.Runtime.InteropServices.RuntimeInformation.IsOSPlatform(
            System.Runtime.InteropServices.OSPlatform.Linux))
        {
            _featureFlags["WE_PLATFORM_LINUX"] = "1";
        }
        else if (System.Runtime.InteropServices.RuntimeInformation.IsOSPlatform(
            System.Runtime.InteropServices.OSPlatform.OSX))
        {
            _featureFlags["WE_PLATFORM_MACOS"] = "1";
        }
        
        // Architecture-specific flags
        var arch = System.Runtime.InteropServices.RuntimeInformation.OSArchitecture;
        if (arch == System.Runtime.InteropServices.Architecture.X64)
        {
            _featureFlags["WE_ARCH_X64"] = "1";
        }
        else if (arch == System.Runtime.InteropServices.Architecture.X86)
        {
            _featureFlags["WE_ARCH_X86"] = "1";
        }
        else if (arch == System.Runtime.InteropServices.Architecture.Arm64)
        {
            _featureFlags["WE_ARCH_ARM64"] = "1";
        }
    }
    
    /// <summary>
    /// Builds the dependency graph.
    /// </summary>
    private void BuildDependencyGraph(Dictionary<string, SDKInfo> sdks, Dictionary<string, ThirdPartyLibrary> libraries)
    {
        _dependencies.Clear();
        
        // Add SDK dependencies
        foreach (var (name, sdk) in sdks)
        {
            var dep = new Dependency
            {
                Name = name,
                Type = DependencyType.SDK,
                IsAvailable = sdk.IsValid,
                IsRequired = IsSDKRequired(name),
                Version = sdk.Version,
                Path = sdk.RootPath,
                IncludePaths = sdk.IncludePaths,
                LibraryPaths = sdk.LibraryPaths,
                CompilerDefinitions = new List<string> { $"WE_HAS_{name.ToUpperInvariant().Replace(" ", "_")}" }
            };
            
            _dependencies[name] = dep;
        }
        
        // Add third-party library dependencies
        foreach (var (name, lib) in libraries)
        {
            var dep = new Dependency
            {
                Name = name,
                Type = DependencyType.ThirdParty,
                IsAvailable = lib.IsValid,
                IsRequired = lib.IsRequired,
                Version = lib.Version,
                Path = lib.RootPath,
                IncludePaths = lib.IncludePaths,
                LibraryPaths = lib.LibraryPaths,
                LibraryNames = lib.LibraryNames,
                CompilerDefinitions = lib.CompilerDefinitions
            };
            
            _dependencies[name] = dep;
        }
    }
    
    /// <summary>
    /// Validates dependencies.
    /// </summary>
    private void ValidateDependencies()
    {
        foreach (var (name, dep) in _dependencies)
        {
            if (dep.IsRequired && !dep.IsAvailable)
            {
                var error = $"Required dependency '{name}' is not available";
                _errors.Add(error);
                Log.Error(error);
            }
            else if (!dep.IsRequired && !dep.IsAvailable)
            {
                var warning = $"Optional dependency '{name}' is not available, related features will be disabled";
                _warnings.Add(warning);
                Log.Warning(warning);
            }
        }
    }
    
    /// <summary>
    /// Checks if an SDK is required.
    /// </summary>
    private bool IsSDKRequired(string sdkName)
    {
        // Core SDKs are required
        return sdkName.ToLowerInvariant() switch
        {
            "windowssdk" or "windows" => true,
            "msvc" => true,
            _ => false
        };
    }
    
    /// <summary>
    /// Checks if a dependency is available.
    /// </summary>
    public bool HasDependency(string name)
    {
        return _dependencies.TryGetValue(name, out var dep) && dep.IsAvailable;
    }
    
    /// <summary>
    /// Gets a dependency by name.
    /// </summary>
    public Dependency? GetDependency(string name)
    {
        return _dependencies.TryGetValue(name, out var dep) ? dep : null;
    }
    
    /// <summary>
    /// Gets all feature flags.
    /// </summary>
    public Dictionary<string, string> GetFeatureFlags()
    {
        return new Dictionary<string, string>(_featureFlags);
    }
    
    /// <summary>
    /// Gets all include paths from available dependencies.
    /// </summary>
    public List<string> GetAllIncludePaths()
    {
        var paths = new List<string>();
        
        foreach (var dep in _dependencies.Values.Where(d => d.IsAvailable))
        {
            paths.AddRange(dep.IncludePaths);
        }
        
        return paths;
    }
    
    /// <summary>
    /// Gets all library paths from available dependencies.
    /// </summary>
    public List<string> GetAllLibraryPaths()
    {
        var paths = new List<string>();
        
        foreach (var dep in _dependencies.Values.Where(d => d.IsAvailable))
        {
            paths.AddRange(dep.LibraryPaths);
        }
        
        return paths;
    }
    
    /// <summary>
    /// Gets all library names from available dependencies.
    /// </summary>
    public List<string> GetAllLibraryNames()
    {
        var names = new List<string>();
        
        foreach (var dep in _dependencies.Values.Where(d => d.IsAvailable))
        {
            names.AddRange(dep.LibraryNames);
        }
        
        return names;
    }
    
    /// <summary>
    /// Gets all compiler definitions from available dependencies.
    /// </summary>
    public List<string> GetAllCompilerDefinitions()
    {
        var definitions = new List<string>();
        
        foreach (var dep in _dependencies.Values.Where(d => d.IsAvailable))
        {
            definitions.AddRange(dep.CompilerDefinitions);
        }
        
        return definitions;
    }
}

/// <summary>
/// Result of dependency resolution.
/// </summary>
public class DependencyResolutionResult
{
    public bool Success { get; set; }
    public Dictionary<string, SDKInfo> SDKs { get; set; } = new();
    public Dictionary<string, ThirdPartyLibrary> ThirdPartyLibraries { get; set; } = new();
    public Dictionary<string, string> FeatureFlags { get; set; } = new();
    public Dictionary<string, Dependency> Dependencies { get; set; } = new();
    public List<string> Warnings { get; set; } = new();
    public List<string> Errors { get; set; } = new();
}

/// <summary>
/// Represents a dependency.
/// </summary>
public class Dependency
{
    public string Name { get; set; } = string.Empty;
    public DependencyType Type { get; set; }
    public bool IsAvailable { get; set; }
    public bool IsRequired { get; set; }
    public string Version { get; set; } = string.Empty;
    public string Path { get; set; } = string.Empty;
    public List<string> IncludePaths { get; set; } = new();
    public List<string> LibraryPaths { get; set; } = new();
    public List<string> LibraryNames { get; set; } = new();
    public List<string> CompilerDefinitions { get; set; } = new();
    public List<string> RequiredBy { get; set; } = new();
}

/// <summary>
/// Dependency type enumeration.
/// </summary>
public enum DependencyType
{
    SDK,
    ThirdParty,
    Module
}
