using System.Text.Json;
using Serilog;

namespace IgniteBT.SDK;

/// <summary>
/// Manages SDK configuration from IgniteBT.SDKs.json.
/// Configuration overrides automatic detection.
/// </summary>
public class SDKConfiguration
{
    private const string ConfigFileName = "IgniteBT.SDKs.json";
    private readonly string[] _searchPaths;
    private Dictionary<string, string> _sdkPaths = new();
    private bool _loaded = false;
    
    public SDKConfiguration()
    {
        _searchPaths = new[]
        {
            ".", // Current directory
            "..", // Parent directory
            "../..", // Grandparent directory
            Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData)
        };
    }
    
    /// <summary>
    /// Loads the SDK configuration file.
    /// </summary>
    public void Load()
    {
        if (_loaded) return;
        
        Log.Information("Loading SDK configuration...");
        
        foreach (var searchPath in _searchPaths)
        {
            var configPath = Path.Combine(searchPath, ConfigFileName);
            if (File.Exists(configPath))
            {
                try
                {
                    var jsonContent = File.ReadAllText(configPath);
                    var config = JsonSerializer.Deserialize<SDKConfigFile>(jsonContent, 
                        new JsonSerializerOptions { PropertyNameCaseInsensitive = true });
                    
                    if (config?.SDKPaths != null)
                    {
                        _sdkPaths = config.SDKPaths;
                        Log.Information("Loaded SDK configuration from: {Path}", configPath);
                        Log.Information("Configured SDKs: {SDKs}", string.Join(", ", _sdkPaths.Keys));
                        _loaded = true;
                        return;
                    }
                }
                catch (Exception ex)
                {
                    Log.Warning(ex, "Failed to load SDK configuration from: {Path}", configPath);
                }
            }
        }
        
        Log.Information("No SDK configuration file found, using automatic detection");
        _loaded = true;
    }
    
    /// <summary>
    /// Gets the configured path for an SDK.
    /// </summary>
    public string? GetSDKPath(string sdkName)
    {
        // Try exact match
        if (_sdkPaths.TryGetValue(sdkName, out var path))
        {
            return path;
        }
        
        // Try case-insensitive match
        var key = _sdkPaths.Keys.FirstOrDefault(k => 
            k.Equals(sdkName, StringComparison.OrdinalIgnoreCase));
        if (!string.IsNullOrEmpty(key))
        {
            return _sdkPaths[key];
        }
        
        return null;
    }
    
    /// <summary>
    /// Checks if an SDK has a manual configuration.
    /// </summary>
    public bool HasManualConfiguration(string sdkName)
    {
        return GetSDKPath(sdkName) != null;
    }
    
    /// <summary>
    /// Gets all configured SDK paths.
    /// </summary>
    public Dictionary<string, string> GetAllConfiguredPaths()
    {
        return new Dictionary<string, string>(_sdkPaths);
    }
    
    /// <summary>
    /// Internal class for JSON deserialization.
    /// </summary>
    private class SDKConfigFile
    {
        public Dictionary<string, string>? SDKPaths { get; set; }
    }
}
