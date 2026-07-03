namespace IgniteBT.Workspace.SDK;

/// <summary>
/// Interface for SDK providers. Each provider is responsible for detecting,
/// validating, and providing information about a specific SDK.
/// </summary>
public interface ISDKProvider
{
    /// <summary>
    /// Gets the name of the SDK this provider handles.
    /// </summary>
    string SDKName { get; }
    
    /// <summary>
    /// Gets the priority of this provider. Higher priority providers are checked first.
    /// </summary>
    int Priority { get; }
    
    /// <summary>
    /// Detects if the SDK is installed and returns its information.
    /// </summary>
    Task<SDKResult<SDKInfo>> DetectAsync();
    
    /// <summary>
    /// Validates the SDK installation at the given path.
    /// </summary>
    Task<SDKResult> ValidateAsync(string path);
    
    /// <summary>
    /// Locates header directories for the SDK.
    /// </summary>
    Task<SDKResult<List<string>>> LocateHeadersAsync(string path);
    
    /// <summary>
    /// Locates library directories for the SDK.
    /// </summary>
    Task<SDKResult<List<string>>> LocateLibrariesAsync(string path);
    
    /// <summary>
    /// Locates binary/executable directories for the SDK.
    /// </summary>
    Task<SDKResult<List<string>>> LocateBinariesAsync(string path);
    
    /// <summary>
    /// Locates tool executables for the SDK.
    /// </summary>
    Task<SDKResult<List<string>>> LocateToolsAsync(string path);
    
    /// <summary>
    /// Gets the version of the SDK at the given path.
    /// </summary>
    Task<SDKResult<string>> GetVersionAsync(string path);
    
    /// <summary>
    /// Checks if the SDK is installed.
    /// </summary>
    Task<bool> IsInstalledAsync();
    
    /// <summary>
    /// Gets the supported platforms for this SDK.
    /// </summary>
    List<TargetPlatform> SupportedPlatforms { get; }
    
    /// <summary>
    /// Gets the supported architectures for this SDK.
    /// </summary>
    List<TargetArchitecture> SupportedArchitectures { get; }
}
