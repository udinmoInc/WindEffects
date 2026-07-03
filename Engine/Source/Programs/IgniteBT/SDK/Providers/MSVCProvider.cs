using Serilog;
using System.Runtime.InteropServices;

namespace IgniteBT.SDK.Providers;

/// <summary>
/// Provider for MSVC (Microsoft Visual C++) compiler.
/// </summary>
public class MSVCProvider : BaseSDKProvider
{
    public override string SDKName => "MSVC";
    public override int Priority => 95; // Very high priority - critical for Windows builds
    
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
    
    protected override List<string> ExpectedTools => new()
    {
        "cl.exe",
        "link.exe",
        "lib.exe"
    };
    
    public override async Task<SDKResult<List<string>>> LocateHeadersAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var headers = new List<string>();
        
        // MSVC headers are typically in VC/Tools/MSVC/<version>/include
        var includePaths = new[]
        {
            Path.Combine(path, "include"),
            Path.Combine(path, "Include"),
            Path.Combine(path, "VC", "Tools", "MSVC", "*", "include"),
            Path.Combine(path, "VC", "include"),
            Path.Combine(path, "VC", "atlmfc", "include")
        };
        
        foreach (var includePattern in includePaths)
        {
            if (includePattern.Contains('*'))
            {
                var directory = Path.GetDirectoryName(includePattern);
                if (directory != null && Directory.Exists(directory))
                {
                    var matchingDirs = Directory.GetDirectories(directory, 
                        Path.GetFileName(includePattern));
                    foreach (var dir in matchingDirs)
                    {
                        var includeDir = Path.Combine(dir, "include");
                        if (Directory.Exists(includeDir))
                        {
                            headers.Add(includeDir);
                        }
                    }
                }
            }
            else if (Directory.Exists(includePattern))
            {
                headers.Add(includePattern);
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
        
        // MSVC libraries are typically in VC/Tools/MSVC/<version>/lib/<arch>
        var libPaths = new[]
        {
            Path.Combine(path, "lib"),
            Path.Combine(path, "Lib"),
            Path.Combine(path, "VC", "Tools", "MSVC", "*", "lib"),
            Path.Combine(path, "VC", "lib"),
            Path.Combine(path, "VC", "atlmfc", "lib")
        };
        
        foreach (var libPattern in libPaths)
        {
            if (libPattern.Contains('*'))
            {
                var directory = Path.GetDirectoryName(libPattern);
                if (directory != null && Directory.Exists(directory))
                {
                    var matchingDirs = Directory.GetDirectories(directory, 
                        Path.GetFileName(libPattern));
                    foreach (var dir in matchingDirs)
                    {
                        var libDir = Path.Combine(dir, "lib");
                        if (Directory.Exists(libDir))
                        {
                            libraries.Add(libDir);
                            
                            // Add architecture-specific subdirectories
                            var archDirs = new[]
                            {
                                Path.Combine(libDir, "x64"),
                                Path.Combine(libDir, "x86"),
                                Path.Combine(libDir, "arm64"),
                                Path.Combine(libDir, "arm")
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
                }
            }
            else if (Directory.Exists(libPattern))
            {
                libraries.Add(libPattern);
            }
        }
        
        result.Success = libraries.Count > 0;
        result.Value = libraries;
        return await Task.FromResult(result);
    }
    
    public override async Task<SDKResult<List<string>>> LocateToolsAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var tools = new List<string>();
        
        // MSVC tools are typically in VC/Tools/MSVC/<version>/bin/<hostarch>/<targetarch>
        var binPaths = new[]
        {
            Path.Combine(path, "bin"),
            Path.Combine(path, "Bin"),
            Path.Combine(path, "VC", "Tools", "MSVC", "*", "bin"),
            Path.Combine(path, "VC", "bin")
        };
        
        foreach (var binPattern in binPaths)
        {
            if (binPattern.Contains('*'))
            {
                var directory = Path.GetDirectoryName(binPattern);
                if (directory != null && Directory.Exists(directory))
                {
                    var matchingDirs = Directory.GetDirectories(directory, 
                        Path.GetFileName(binPattern));
                    foreach (var dir in matchingDirs)
                    {
                        var binDir = Path.Combine(dir, "bin");
                        if (Directory.Exists(binDir))
                        {
                            // Add all architecture-specific bin directories
                            var hostArchDirs = Directory.GetDirectories(binDir);
                            foreach (var hostArchDir in hostArchDirs)
                            {
                                tools.Add(hostArchDir);
                                
                                // Add target architecture subdirectories
                                var targetArchDirs = Directory.GetDirectories(hostArchDir);
                                foreach (var targetArchDir in targetArchDirs)
                                {
                                    tools.Add(targetArchDir);
                                }
                            }
                        }
                    }
                }
            }
            else if (Directory.Exists(binPattern))
            {
                tools.Add(binPattern);
            }
        }
        
        result.Success = tools.Count > 0;
        result.Value = tools;
        return await Task.FromResult(result);
    }
    
    public override async Task<SDKResult<string>> GetVersionAsync(string path)
    {
        // Try to extract version from path (e.g., 14.44.35207)
        var dirName = new DirectoryInfo(path).Name;
        if (System.Version.TryParse(dirName, out _))
        {
            return await Task.FromResult(SDKResult<string>.Ok(dirName));
        }
        
        // Try to find version in VC/Tools/MSVC/<version>
        var msvcPath = Path.Combine(path, "VC", "Tools", "MSVC");
        if (Directory.Exists(msvcPath))
        {
            var versionDirs = Directory.GetDirectories(msvcPath);
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
