using Serilog;
using System.Collections.Concurrent;

namespace IgniteBT.Workspace.SDK;

/// <summary>
/// Central manager for SDK discovery, validation, and resolution.
/// Thread-safe singleton with caching.
/// </summary>
public class SDKManager
{
    private static readonly Lazy<SDKManager> _instance = new(() => new SDKManager());
    public static SDKManager Instance => _instance.Value;
    
    private readonly List<ISDKProvider> _providers = new();
    private readonly ConcurrentDictionary<string, SDKInfo> _cache = new();
    private readonly SDKConfiguration _configuration;
    private readonly SDKDatabase _database;
    private readonly SDKValidator _validator;
    private readonly SDKResolver _resolver;
    private readonly object _lock = new();
    private bool _initialized = false;
    
    private SDKManager()
    {
        _configuration = new SDKConfiguration();
        _database = new SDKDatabase();
        _validator = new SDKValidator();
        _resolver = new SDKResolver(_configuration, _database);
    }
    
    /// <summary>
    /// Initializes the SDK manager with all available providers.
    /// </summary>
    public void Initialize()
    {
        lock (_lock)
        {
            if (_initialized) return;
            
            Log.Information("Initializing SDK Manager...");
            
            // Load configuration
            _configuration.Load();
            
            // Register providers in priority order
            RegisterProviders();
            
            // Load cached SDKs from database
            _database.LoadCache(_cache);
            
            _initialized = true;
            Log.Information("SDK Manager initialized with {Count} providers", _providers.Count);
        }
    }
    
    /// <summary>
    /// Registers all SDK providers.
    /// </summary>
    private void RegisterProviders()
    {
        _providers.Clear();
        
        // Register providers sorted by priority (highest first)
        var providers = new List<ISDKProvider>
        {
            // Core toolchain providers (highest priority)
            new Providers.MSVCProvider(),
            new Providers.WindowsSDKProvider(),
            
            // Graphics SDKs
            new Providers.VulkanSDKProvider(),
            new Providers.SDL3Provider(),
            new Providers.DirectXProvider(),
            new Providers.OpenXRProvider(),
            
            // Runtime SDKs
            new Providers.DotNetProvider(),
            new Providers.PythonProvider(),
            
            // Platform SDKs
            new Providers.AndroidProvider()
        };
        
        _providers.AddRange(providers.OrderByDescending(p => p.Priority));
        
        Log.Debug("Registered {Count} SDK providers", _providers.Count);
    }
    
    /// <summary>
    /// Detects all available SDKs. Uses persistent cache when fingerprint is unchanged.
    /// </summary>
    public async Task<Dictionary<string, SDKInfo>> DetectAllAsync(string? compilerPath = null, bool forceRescan = false)
    {
        Initialize();

        if (!forceRescan && _cache.Count > 0)
        {
            if (SDKCacheFingerprint.IsCacheValid(compilerPath) && ValidateCachedPaths())
            {
                Log.Information("SDK cache hit — using {Count} cached SDKs (fingerprint valid)", _cache.Count);
                return new Dictionary<string, SDKInfo>(_cache);
            }
        }

        Log.Information("Detecting all SDKs...");
        var results = new Dictionary<string, SDKInfo>();
        var stopwatch = System.Diagnostics.Stopwatch.StartNew();

        var detectTasks = _providers.Select(async provider =>
        {
            try
            {
                var result = await DetectSDKAsync(provider);
                return (provider.SDKName, result);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Failed to detect SDK: {SDKName}", provider.SDKName);
                return (provider.SDKName, SDKResult<SDKInfo>.Fail(ex.Message));
            }
        });

        foreach (var task in detectTasks)
        {
            var (name, result) = await task;
            if (result.Success && result.Value != null)
            {
                results[name] = result.Value;
                _cache[name] = result.Value;
            }
        }

        _database.SaveCache(_cache);
        SDKCacheFingerprint.SaveFingerprint(SDKCacheFingerprint.Compute(compilerPath));

        stopwatch.Stop();
        Log.Information("Detected {Count} SDKs in {Duration}ms", results.Count, stopwatch.ElapsedMilliseconds);

        return results;
    }

    private bool ValidateCachedPaths()
    {
        foreach (var (_, info) in _cache)
        {
            if (!info.IsValid) return false;
            if (string.IsNullOrEmpty(info.RootPath) || !Directory.Exists(info.RootPath))
                return false;
        }
        return true;
    }

    /// <summary>
    /// Detects all available SDKs (legacy sequential path for forced rescan).
    /// </summary>
    public async Task<Dictionary<string, SDKInfo>> DetectAllSequentialAsync()
    {
        Initialize();
        
        Log.Information("Detecting all SDKs...");
        var results = new Dictionary<string, SDKInfo>();
        var stopwatch = System.Diagnostics.Stopwatch.StartNew();
        
        foreach (var provider in _providers)
        {
            try
            {
                var result = await DetectSDKAsync(provider);
                if (result.Success && result.Value != null)
                {
                    results[provider.SDKName] = result.Value;
                    _cache[provider.SDKName] = result.Value;
                }
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Failed to detect SDK: {SDKName}", provider.SDKName);
            }
        }
        
        // Save cache
        _database.SaveCache(_cache);
        
        stopwatch.Stop();
        Log.Information("Detected {Count} SDKs in {Duration}ms", results.Count, stopwatch.ElapsedMilliseconds);
        
        return results;
    }
    
    /// <summary>
    /// Detects a specific SDK by name.
    /// </summary>
    public async Task<SDKResult<SDKInfo>> DetectSDKAsync(string sdkName)
    {
        Initialize();
        
        // Check cache first
        if (_cache.TryGetValue(sdkName, out var cachedInfo))
        {
            Log.Debug("SDK {SDKName} found in cache", sdkName);
            return SDKResult<SDKInfo>.Ok(cachedInfo);
        }
        
        // Find provider
        var provider = _providers.FirstOrDefault(p => 
            p.SDKName.Equals(sdkName, StringComparison.OrdinalIgnoreCase));
        
        if (provider == null)
        {
            return SDKResult<SDKInfo>.Fail($"No provider found for SDK: {sdkName}");
        }
        
        return await DetectSDKAsync(provider);
    }
    
    /// <summary>
    /// Detects SDK using a specific provider.
    /// </summary>
    private async Task<SDKResult<SDKInfo>> DetectSDKAsync(ISDKProvider provider)
    {
        Log.Information("Detecting SDK: {SDKName}", provider.SDKName);
        var stopwatch = System.Diagnostics.Stopwatch.StartNew();
        
        // Use resolver to find SDK path
        var resolveResult = await _resolver.ResolveAsync(provider);
        
        if (!resolveResult.Success || string.IsNullOrEmpty(resolveResult.Value))
        {
            stopwatch.Stop();
            Log.Warning("SDK {SDKName} not found", provider.SDKName);
            var result = SDKResult<SDKInfo>.Fail($"SDK {provider.SDKName} not found");
            result.SearchLocations.AddRange(resolveResult.SearchLocations);
            return result;
        }
        
        var path = resolveResult.Value;
        
        // Build SDK info
        var info = new SDKInfo
        {
            Name = provider.SDKName,
            RootPath = path,
            DiscoverySource = resolveResult.DiscoverySource ?? "Unknown",
            Platform = GetCurrentPlatform(),
            Architecture = GetCurrentArchitecture()
        };
        
        // Get version
        var versionResult = await provider.GetVersionAsync(path);
        if (versionResult.Success)
        {
            info.Version = versionResult.Value ?? "Unknown";
        }
        
        // Locate paths
        var headersResult = await provider.LocateHeadersAsync(path);
        if (headersResult.Success)
        {
            info.IncludePaths = headersResult.Value ?? new List<string>();
        }
        
        var librariesResult = await provider.LocateLibrariesAsync(path);
        if (librariesResult.Success)
        {
            info.LibraryPaths = librariesResult.Value ?? new List<string>();
        }
        
        var binariesResult = await provider.LocateBinariesAsync(path);
        if (binariesResult.Success)
        {
            info.BinaryPaths = binariesResult.Value ?? new List<string>();
        }
        
        var toolsResult = await provider.LocateToolsAsync(path);
        if (toolsResult.Success)
        {
            info.ToolPaths = toolsResult.Value ?? new List<string>();
        }
        
        // Validate
        var validationResult = await _validator.ValidateAsync(provider, info);
        info.IsValid = validationResult.Success;
        info.ValidationErrors = validationResult.ValidationErrors;
        info.Warnings = validationResult.Warnings;
        
        stopwatch.Stop();
        info.DetectionDuration = stopwatch.Elapsed;
        info.DetectionTime = DateTime.UtcNow;
        
        if (info.IsValid)
        {
            Log.Information("SDK {SDKName} v{Version} detected at {Path} ({Source})", 
                info.Name, info.Version, info.RootPath, info.DiscoverySource);
        }
        else
        {
            Log.Warning("SDK {SDKName} detected but validation failed", info.Name);
        }
        
        return SDKResult<SDKInfo>.Ok(info);
    }
    
    /// <summary>
    /// Gets SDK information by name.
    /// </summary>
    public SDKResult<SDKInfo> GetSDK(string sdkName)
    {
        Initialize();
        
        if (_cache.TryGetValue(sdkName, out var info))
        {
            return SDKResult<SDKInfo>.Ok(info);
        }
        
        return SDKResult<SDKInfo>.Fail($"SDK not found in cache: {sdkName}");
    }
    
    /// <summary>
    /// Gets all cached SDKs.
    /// </summary>
    public Dictionary<string, SDKInfo> GetAllSDKs()
    {
        Initialize();
        return new Dictionary<string, SDKInfo>(_cache);
    }
    
    /// <summary>
    /// Clears the SDK cache.
    /// </summary>
    public void ClearCache()
    {
        lock (_lock)
        {
            _cache.Clear();
            _database.ClearCache();
            Log.Information("SDK cache cleared");
        }
    }
    
    /// <summary>
    /// Forces a re-scan of all SDKs.
    /// </summary>
    public async Task<Dictionary<string, SDKInfo>> RescanAsync(string? compilerPath = null)
    {
        ClearCache();
        return await DetectAllAsync(compilerPath, forceRescan: true);
    }
    
    /// <summary>
    /// Validates all cached SDKs.
    /// </summary>
    public async Task<Dictionary<string, SDKResult>> ValidateAllAsync()
    {
        Initialize();
        
        var results = new Dictionary<string, SDKResult>();
        
        foreach (var (name, info) in _cache)
        {
            var provider = _providers.FirstOrDefault(p => 
                p.SDKName.Equals(name, StringComparison.OrdinalIgnoreCase));
            
            if (provider != null)
            {
                var result = await _validator.ValidateAsync(provider, info);
                results[name] = new SDKResult
                {
                    Success = result.Success,
                    Error = result.ValidationErrors.Count > 0 ? 
                        string.Join("; ", result.ValidationErrors) : null,
                    Warnings = result.Warnings
                };
            }
        }
        
        return results;
    }
    
    /// <summary>
    /// Gets the current platform.
    /// </summary>
    private TargetPlatform GetCurrentPlatform()
    {
        if (System.Runtime.InteropServices.RuntimeInformation.IsOSPlatform(
            System.Runtime.InteropServices.OSPlatform.Windows))
            return TargetPlatform.Windows;
        if (System.Runtime.InteropServices.RuntimeInformation.IsOSPlatform(
            System.Runtime.InteropServices.OSPlatform.Linux))
            return TargetPlatform.Linux;
        if (System.Runtime.InteropServices.RuntimeInformation.IsOSPlatform(
            System.Runtime.InteropServices.OSPlatform.OSX))
            return TargetPlatform.macOS;
        
        return TargetPlatform.Unknown;
    }
    
    /// <summary>
    /// Gets the current architecture.
    /// </summary>
    private TargetArchitecture GetCurrentArchitecture()
    {
        var arch = System.Runtime.InteropServices.RuntimeInformation.OSArchitecture;
        return arch switch
        {
            System.Runtime.InteropServices.Architecture.X64 => TargetArchitecture.x64,
            System.Runtime.InteropServices.Architecture.X86 => TargetArchitecture.x86,
            System.Runtime.InteropServices.Architecture.Arm64 => TargetArchitecture.ARM64,
            System.Runtime.InteropServices.Architecture.Arm => TargetArchitecture.ARM,
            _ => TargetArchitecture.Unknown
        };
    }
}
