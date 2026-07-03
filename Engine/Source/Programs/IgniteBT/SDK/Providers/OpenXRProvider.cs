using Serilog;
using System.Runtime.InteropServices;

namespace IgniteBT.SDK.Providers;

/// <summary>
/// Provider for OpenXR SDK.
/// </summary>
public class OpenXRProvider : BaseSDKProvider
{
    public override string SDKName => "OpenXR";
    public override int Priority => 45;
    
    public override List<TargetPlatform> SupportedPlatforms => new()
    {
        TargetPlatform.Windows,
        TargetPlatform.Linux,
        TargetPlatform.Android
    };
    
    public override List<TargetArchitecture> SupportedArchitectures => new()
    {
        TargetArchitecture.x64,
        TargetArchitecture.ARM64
    };
    
    protected override List<string> EnvironmentVariableNames => new()
    {
        "OPENXR_SDK"
    };
    
    protected override List<string> DefaultPaths => new()
    {
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "OpenXR"),
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "OpenXR"),
        "/usr/local/include/openxr",
        "/usr/include/openxr",
        "/opt/openxr"
    };
    
    protected override List<string> ExpectedHeaders => new()
    {
        "openxr/openxr.h",
        "openxr/openxr_platform.h"
    };
    
    protected override List<string> ExpectedLibraries => new()
    {
        "openxr_loader.lib",
        "libopenxr_loader.so",
        "libopenxr_loader.dylib"
    };
    
    public override async Task<SDKResult<List<string>>> LocateHeadersAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var headers = new List<string>();
        
        // OpenXR SDK structure
        var includePaths = new[]
        {
            Path.Combine(path, "include"),
            Path.Combine(path, "Include"),
            Path.Combine(path, "include", "openxr"),
            Path.Combine(path, "Include", "openxr")
        };
        
        foreach (var includePath in includePaths)
        {
            if (Directory.Exists(includePath))
            {
                headers.Add(includePath);
                
                // Also add parent directory if it contains openxr subdirectory
                var parentDir = Directory.GetParent(includePath)?.FullName;
                if (parentDir != null && Directory.Exists(parentDir))
                {
                    headers.Add(parentDir);
                }
            }
        }
        
        result.Success = headers.Count > 0;
        result.Value = headers;
        return await Task.FromResult(result);
    }
    
    public override async Task<SDKResult<List<string>>> LocateLibrariesAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var libraries = new List<string>();
        
        // OpenXR SDK library structure
        var libPaths = new[]
        {
            Path.Combine(path, "lib"),
            Path.Combine(path, "Lib"),
            Path.Combine(path, "src", "loader"),
            Path.Combine(path, "build", "src", "loader")
        };
        
        foreach (var libPath in libPaths)
        {
            if (Directory.Exists(libPath))
            {
                libraries.Add(libPath);
                
                // Add architecture-specific subdirectories
                var archDirs = new[]
                {
                    Path.Combine(libPath, "x64"),
                    Path.Combine(libPath, "x86"),
                    Path.Combine(libPath, "arm64")
                };
                
                foreach (var archDir in archDirs)
                {
                    if (Directory.Exists(archDir))
                    {
                        libraries.Add(archDir);
                    }
                }
            }
        }
        
        result.Success = libraries.Count > 0;
        result.Value = libraries;
        return await Task.FromResult(result);
    }
}
