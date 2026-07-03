using Serilog;

namespace IgniteBT.SDK;

/// <summary>
/// Scans for SDKs in IgniteBT.SDKs.json configuration file.
/// </summary>
public class JsonScanner
{
    private readonly SDKConfiguration _configuration;
    
    public JsonScanner(SDKConfiguration configuration)
    {
        _configuration = configuration;
    }
    
    /// <summary>
    /// Scans for the SDK in the configuration file.
    /// </summary>
    public async Task<SDKScanResult> ScanAsync(ISDKProvider provider)
    {
        var result = new SDKScanResult();
        
        if (!_configuration.HasManualConfiguration(provider.SDKName))
        {
            return result;
        }
        
        var configuredPath = _configuration.GetSDKPath(provider.SDKName);
        if (string.IsNullOrEmpty(configuredPath))
        {
            return result;
        }
        
        result.SearchLocations.Add(configuredPath);
        
        if (Directory.Exists(configuredPath))
        {
            result.Success = true;
            result.Value = configuredPath;
            Log.Debug("Found {SDKName} in configuration: {Path}", provider.SDKName, configuredPath);
        }
        else
        {
            Log.Warning("Configured path for {SDKName} does not exist: {Path}", 
                provider.SDKName, configuredPath);
        }
        
        return await Task.FromResult(result);
    }
}
