namespace IgniteBT.Workspace.SDK;

/// <summary>
/// Represents comprehensive information about an SDK.
/// </summary>
public class SDKInfo
{
    public string Name { get; set; } = string.Empty;
    public string Version { get; set; } = string.Empty;
    public string RootPath { get; set; } = string.Empty;
    public string DiscoverySource { get; set; } = string.Empty;
    public TargetArchitecture Architecture { get; set; }
    public TargetPlatform Platform { get; set; }
    
    public List<string> IncludePaths { get; set; } = new();
    public List<string> LibraryPaths { get; set; } = new();
    public List<string> BinaryPaths { get; set; } = new();
    public List<string> ToolPaths { get; set; } = new();
    
    public Dictionary<string, string> EnvironmentVariables { get; set; } = new();
    public Dictionary<string, string> Properties { get; set; } = new();
    
    public bool IsValid { get; set; }
    public List<string> ValidationErrors { get; set; } = new();
    public List<string> Warnings { get; set; } = new();
    
    public DateTime DetectionTime { get; set; } = DateTime.UtcNow;
    public TimeSpan DetectionDuration { get; set; }
}

/// <summary>
/// Target architecture enumeration.
/// </summary>
public enum TargetArchitecture
{
    x86,
    x64,
    ARM,
    ARM64,
    Unknown
}

/// <summary>
/// Target platform enumeration.
/// </summary>
public enum TargetPlatform
{
    Windows,
    Linux,
    macOS,
    Android,
    iOS,
    Unknown
}
