using Serilog;
using System.Runtime.InteropServices;

namespace IgniteBT.Workspace.SDK.Providers;

/// <summary>
/// Provider for Windows SDK.
/// </summary>
public class WindowsSDKProvider : BaseSDKProvider
{
    public override string SDKName => "WindowsSDK";
    public override int Priority => 100; // Highest priority - critical for Windows builds
    
    public override List<TargetPlatform> SupportedPlatforms => new()
    {
        TargetPlatform.Windows
    };
    
    public override List<TargetArchitecture> SupportedArchitectures => new()
    {
        TargetArchitecture.x64,
        TargetArchitecture.x86,
        TargetArchitecture.ARM64
    };
    
    protected override List<string> EnvironmentVariableNames => new()
    {
        "WindowsSdkDir",
        "WINDOWSSDKDIR"
    };
    
    protected override List<string> ExpectedHeaders => new()
    {
        "windows.h",
        "windowsx.h",
        "windef.h"
    };
    
    protected override List<string> ExpectedLibraries => new()
    {
        "kernel32.lib",
        "user32.lib",
        "gdi32.lib"
    };
    
    public override async Task<SDKResult<List<string>>> LocateHeadersAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var headers = new List<string>();
        
        // Windows SDK has a specific structure: Windows Kits/10/Include/<version>
        var includePath = Path.Combine(path, "Include");
        if (Directory.Exists(includePath))
        {
            // Find all version directories
            var versionDirs = Directory.GetDirectories(includePath);
            foreach (var versionDir in versionDirs)
            {
                headers.Add(versionDir);
                headers.Add(Path.Combine(versionDir, "um"));
                headers.Add(Path.Combine(versionDir, "shared"));
                headers.Add(Path.Combine(versionDir, "winrt"));
            }
            
            // Also add the latest version's um and shared directories
            if (versionDirs.Length > 0)
            {
                var latestVersion = versionDirs.OrderByDescending(d => d).First();
                headers.Add(Path.Combine(latestVersion, "um"));
                headers.Add(Path.Combine(latestVersion, "shared"));
                headers.Add(Path.Combine(latestVersion, "winrt"));
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
        
        // Windows SDK library structure: Windows Kits/10/Lib/<version>/<arch>
        var libPath = Path.Combine(path, "Lib");
        if (Directory.Exists(libPath))
        {
            var versionDirs = Directory.GetDirectories(libPath);
            foreach (var versionDir in versionDirs)
            {
                // Add architecture-specific library directories
                var archDirs = new[]
                {
                    Path.Combine(versionDir, "x64"),
                    Path.Combine(versionDir, "x86"),
                    Path.Combine(versionDir, "arm"),
                    Path.Combine(versionDir, "arm64"),
                    Path.Combine(versionDir, "um"),
                    Path.Combine(versionDir, "ucrt")
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
    
    public override async Task<SDKResult<string>> GetVersionAsync(string path)
    {
        // Windows SDK version is typically in the directory name
        // Try to find the latest version in Include directory
        var includePath = Path.Combine(path, "Include");
        if (Directory.Exists(includePath))
        {
            var versionDirs = Directory.GetDirectories(includePath);
            if (versionDirs.Length > 0)
            {
                var latestVersion = new DirectoryInfo(versionDirs.OrderByDescending(d => d).First()).Name;
                return await Task.FromResult(SDKResult<string>.Ok(latestVersion));
            }
        }
        
        return await Task.FromResult(SDKResult<string>.Ok("Unknown"));
    }
    
    public override async Task<bool> IsInstalledAsync()
    {
        if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            return false;
        }
        
        return await base.IsInstalledAsync();
    }
}
