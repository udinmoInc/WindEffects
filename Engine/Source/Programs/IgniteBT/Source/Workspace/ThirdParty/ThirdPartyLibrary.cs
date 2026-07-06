using IgniteBT.Workspace.SDK;

namespace IgniteBT.Workspace.ThirdParty;

/// <summary>
/// Represents a third-party library dependency.
/// </summary>
public class ThirdPartyLibrary
{
    public string Name { get; set; } = string.Empty;
    public string Version { get; set; } = string.Empty;
    public string RootPath { get; set; } = string.Empty;
    
    public List<string> IncludePaths { get; set; } = new();
    public List<string> LibraryPaths { get; set; } = new();
    public List<string> BinaryPaths { get; set; } = new();
    public List<string> LibraryNames { get; set; } = new();
    
    public List<TargetPlatform> SupportedPlatforms { get; set; } = new();
    public List<TargetArchitecture> SupportedArchitectures { get; set; } = new();
    
    public List<string> CompilerDefinitions { get; set; } = new();
    public Dictionary<string, string> Properties { get; set; } = new();
    
    public bool IsValid { get; set; }
    public List<string> ValidationErrors { get; set; } = new();
    public List<string> Warnings { get; set; } = new();
    
    public bool IsRequired { get; set; } = true;
    public List<string> RequiredBy { get; set; } = new();
}
