using Serilog;

namespace IgniteBT.Workspace.SDK;

/// <summary>
/// Resolves SDK paths using multiple discovery sources in priority order.
/// </summary>
public class SDKResolver
{
    private readonly SDKConfiguration _configuration;
    private readonly SDKDatabase _database;
    
    private readonly JsonScanner _jsonScanner;
    private readonly EnvironmentScanner _environmentScanner;
    private readonly RegistryScanner _registryScanner;
    private readonly FileSystemScanner _fileSystemScanner;
    
    public SDKResolver(SDKConfiguration configuration, SDKDatabase database)
    {
        _configuration = configuration;
        _database = database;
        
        _jsonScanner = new JsonScanner(configuration);
        _environmentScanner = new EnvironmentScanner();
        _registryScanner = new RegistryScanner();
        _fileSystemScanner = new FileSystemScanner();
    }
    
    /// <summary>
    /// Resolves the SDK path using all discovery sources in priority order.
    /// </summary>
    public async Task<SDKResolveResult> ResolveAsync(ISDKProvider provider)
    {
        var result = new SDKResolveResult();
        var stopwatch = System.Diagnostics.Stopwatch.StartNew();
        
        Log.Debug("Resolving SDK: {SDKName}", provider.SDKName);
        
        // 1. Check IgniteBT.SDKs.json (highest priority)
        var jsonResult = await _jsonScanner.ScanAsync(provider);
        result.SearchLocations.AddRange(jsonResult.SearchLocations);
        if (jsonResult.Success)
        {
            result.Value = jsonResult.Value;
            result.DiscoverySource = "IgniteBT.SDKs.json";
            Log.Debug("SDK {SDKName} found in configuration file", provider.SDKName);
            return result;
        }
        
        // 2. Check Environment Variables
        var envResult = await _environmentScanner.ScanAsync(provider);
        result.SearchLocations.AddRange(envResult.SearchLocations);
        if (envResult.Success)
        {
            result.Value = envResult.Value;
            result.DiscoverySource = "Environment Variable";
            Log.Debug("SDK {SDKName} found via environment variable", provider.SDKName);
            return result;
        }
        
        // 3. Check Windows Registry (Windows only)
        if (System.Runtime.InteropServices.RuntimeInformation.IsOSPlatform(
            System.Runtime.InteropServices.OSPlatform.Windows))
        {
            var registryResult = await _registryScanner.ScanAsync(provider);
            result.SearchLocations.AddRange(registryResult.SearchLocations);
            if (registryResult.Success)
            {
                result.Value = registryResult.Value;
                result.DiscoverySource = "Windows Registry";
                Log.Debug("SDK {SDKName} found via registry", provider.SDKName);
                return result;
            }
        }
        
        // 4. Check Default Installation Locations
        var fsResult = await _fileSystemScanner.ScanAsync(provider);
        result.SearchLocations.AddRange(fsResult.SearchLocations);
        if (fsResult.Success)
        {
            result.Value = fsResult.Value;
            result.DiscoverySource = "Default Installation";
            Log.Debug("SDK {SDKName} found in default location", provider.SDKName);
            return result;
        }
        
        // 5. Check Engine/ThirdParty
        var thirdPartyResult = await ScanThirdPartyAsync(provider);
        result.SearchLocations.AddRange(thirdPartyResult.SearchLocations);
        if (thirdPartyResult.Success)
        {
            result.Value = thirdPartyResult.Value;
            result.DiscoverySource = "Engine/ThirdParty";
            Log.Debug("SDK {SDKName} found in Engine/ThirdParty", provider.SDKName);
            return result;
        }

        // 6. Check bundled Vulkan headers in Engine/ThirdParty
        if (provider.SDKName.Equals("VulkanSDK", StringComparison.OrdinalIgnoreCase))
        {
            var bundledResult = await ScanBundledVulkanAsync();
            result.SearchLocations.AddRange(bundledResult.SearchLocations);
            if (bundledResult.Success)
            {
                result.Value = bundledResult.Value;
                result.DiscoverySource = "Engine/ThirdParty (bundled)";
                Log.Debug("SDK {SDKName} found via bundled ThirdParty headers", provider.SDKName);
                return result;
            }
        }
        
        // 7. SDK not found
        stopwatch.Stop();
        Log.Warning("SDK {SDKName} not found after checking {Count} locations", 
            provider.SDKName, result.SearchLocations.Count);
        
        return result;
    }
    
    /// <summary>
    /// Scans Engine/ThirdParty directory for the SDK.
    /// </summary>
    private async Task<SDKScanResult> ScanThirdPartyAsync(ISDKProvider provider)
    {
        var result = new SDKScanResult();
        
        // Try to find engine root
        var engineRoot = FindEngineRoot();
        if (string.IsNullOrEmpty(engineRoot))
        {
            return result;
        }
        
        var thirdPartyPath = Path.Combine(engineRoot, "ThirdParty", provider.SDKName);
        result.SearchLocations.Add(thirdPartyPath);
        
        if (Directory.Exists(thirdPartyPath))
        {
            result.Success = true;
            result.Value = thirdPartyPath;
        }
        
        return await Task.FromResult(result);
    }

    /// <summary>
    /// Detects bundled Vulkan headers and loader under Engine/ThirdParty.
    /// </summary>
    private async Task<SDKScanResult> ScanBundledVulkanAsync()
    {
        var result = new SDKScanResult();
        var engineRoot = FindEngineRoot();
        if (string.IsNullOrEmpty(engineRoot))
        {
            return result;
        }

        var thirdPartyRoot = Path.Combine(engineRoot, "ThirdParty");
        var vulkanHeaders = Path.Combine(thirdPartyRoot, "Vulkan-Headers", "include", "vulkan", "vulkan.h");
        var volkHeader = Path.Combine(thirdPartyRoot, "volk", "volk.h");
        var loaderLib = Path.Combine(thirdPartyRoot, "Vulkan-Loader", "build", "loader", "Release", "vulkan-1.lib");

        result.SearchLocations.Add(vulkanHeaders);
        result.SearchLocations.Add(volkHeader);
        result.SearchLocations.Add(loaderLib);

        if (File.Exists(vulkanHeaders) && File.Exists(volkHeader))
        {
            result.Success = true;
            result.Value = thirdPartyRoot;
        }

        return await Task.FromResult(result);
    }
    
    /// <summary>
    /// Attempts to find the engine root directory ({repo}/Engine).
    /// </summary>
    private string? FindEngineRoot()
    {
        return IgniteBT.Build.Layout.BuildLayout.FindEngineRoot(Directory.GetCurrentDirectory());
    }
}

/// <summary>
/// Result of SDK resolution.
/// </summary>
public class SDKResolveResult
{
    public bool Success => !string.IsNullOrEmpty(Value);
    public string? Value { get; set; }
    public string? DiscoverySource { get; set; }
    public List<string> SearchLocations { get; set; } = new();
}

/// <summary>
/// Result of SDK scanning.
/// </summary>
public class SDKScanResult
{
    public bool Success { get; set; }
    public string? Value { get; set; }
    public List<string> SearchLocations { get; set; } = new();
}
