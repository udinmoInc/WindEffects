using System.Text.Json;
using Serilog;

namespace IgniteBT.Toolchain;

/// <summary>
/// SDK configuration loaded from a JSON file.
/// </summary>
public class SDKConfig
{
    public Dictionary<string, string> SDKPaths { get; set; } = new();
    public Dictionary<string, List<string>> AdditionalIncludePaths { get; set; } = new();
    public Dictionary<string, List<string>> AdditionalLibraryPaths { get; set; } = new();

    private static readonly string ConfigFileName = "IgniteBT.SDKs.json";
    private static readonly string[] ConfigSearchPaths = 
    {
        ".", // Current directory
        "..", // Parent directory
        "../..", // Grandparent directory
        Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData)
    };

    /// <summary>
    /// Loads SDK configuration from file.
    /// </summary>
    public static SDKConfig Load()
    {
        var config = new SDKConfig();

        foreach (var searchPath in ConfigSearchPaths)
        {
            var configPath = Path.Combine(searchPath, ConfigFileName);
            if (File.Exists(configPath))
            {
                try
                {
                    var jsonContent = File.ReadAllText(configPath);
                    config = JsonSerializer.Deserialize<SDKConfig>(jsonContent) ?? new SDKConfig();
                    Log.Information("Loaded SDK configuration from: {Path}", configPath);
                    return config;
                }
                catch (Exception ex)
                {
                    Log.Warning(ex, "Failed to load SDK configuration from: {Path}", configPath);
                }
            }
        }

        Log.Information("No SDK configuration file found, using defaults");
        return config;
    }

    /// <summary>
    /// Gets the path for a specific SDK.
    /// </summary>
    public string? GetSDKPath(string sdkName)
    {
        return SDKPaths.TryGetValue(sdkName, out var path) ? path : null;
    }

    /// <summary>
    /// Gets additional include paths for a module.
    /// </summary>
    public List<string> GetAdditionalIncludePaths(string moduleName)
    {
        return AdditionalIncludePaths.TryGetValue(moduleName, out var paths) ? paths : new List<string>();
    }

    /// <summary>
    /// Gets additional library paths for a module.
    /// </summary>
    public List<string> GetAdditionalLibraryPaths(string moduleName)
    {
        return AdditionalLibraryPaths.TryGetValue(moduleName, out var paths) ? paths : new List<string>();
    }
}
