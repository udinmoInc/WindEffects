using Serilog;
using System.Runtime.InteropServices;

namespace IgniteBT.Workspace.SDK.Providers;

/// <summary>
/// Base class for SDK providers with common functionality.
/// </summary>
public abstract class BaseSDKProvider : ISDKProvider
{
    public abstract string SDKName { get; }
    public abstract int Priority { get; }
    public abstract List<TargetPlatform> SupportedPlatforms { get; }
    public abstract List<TargetArchitecture> SupportedArchitectures { get; }
    
    /// <summary>
    /// Gets the default environment variable names for this SDK.
    /// </summary>
    protected virtual List<string> EnvironmentVariableNames => new();
    
    /// <summary>
    /// Gets the default installation paths for this SDK.
    /// </summary>
    protected virtual List<string> DefaultPaths => new();
    
    /// <summary>
    /// Gets the expected header files for validation.
    /// </summary>
    protected virtual List<string> ExpectedHeaders => new();
    
    /// <summary>
    /// Gets the expected library files for validation.
    /// </summary>
    protected virtual List<string> ExpectedLibraries => new();
    
    /// <summary>
    /// Gets the expected tool executables for validation.
    /// </summary>
    protected virtual List<string> ExpectedTools => new();
    
    public virtual async Task<SDKResult<SDKInfo>> DetectAsync()
    {
        var manager = SDKManager.Instance;
        return await manager.DetectSDKAsync(SDKName);
    }
    
    public virtual async Task<SDKResult> ValidateAsync(string path)
    {
        var result = new SDKResult();
        
        if (!Directory.Exists(path))
        {
            return SDKResult.Fail($"Path does not exist: {path}");
        }
        
        // Check for expected headers
        foreach (var header in ExpectedHeaders)
        {
            if (!FindFileInPath(path, header))
            {
                result.Warnings.Add($"Expected header not found: {header}");
            }
        }
        
        // Check for expected libraries
        foreach (var library in ExpectedLibraries)
        {
            if (!FindFileInPath(path, library))
            {
                result.Warnings.Add($"Expected library not found: {library}");
            }
        }
        
        // Check for expected tools
        foreach (var tool in ExpectedTools)
        {
            if (!FindFileInPath(path, tool))
            {
                result.Warnings.Add($"Expected tool not found: {tool}");
            }
        }
        
        result.Success = true;
        return await Task.FromResult(result);
    }
    
    public virtual async Task<SDKResult<List<string>>> LocateHeadersAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var headers = new List<string>();
        
        // Common include directories
        var includeDirs = new[]
        {
            Path.Combine(path, "Include"),
            Path.Combine(path, "include"),
            Path.Combine(path, "inc"),
            Path.Combine(path, "headers"),
            Path.Combine(path, "include", "public"),
            path
        };
        
        foreach (var dir in includeDirs)
        {
            if (Directory.Exists(dir))
            {
                headers.Add(dir);
            }
        }
        
        result.Success = headers.Count > 0;
        result.Value = headers;
        return await Task.FromResult(result);
    }
    
    public virtual async Task<SDKResult<List<string>>> LocateLibrariesAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var libraries = new List<string>();
        
        // Common library directories
        var libDirs = new[]
        {
            Path.Combine(path, "Lib"),
            Path.Combine(path, "lib"),
            Path.Combine(path, "libs"),
            Path.Combine(path, "Library"),
            Path.Combine(path, "library"),
            Path.Combine(path, "Libraries"),
            Path.Combine(path, "libraries")
        };
        
        foreach (var dir in libDirs)
        {
            if (Directory.Exists(dir))
            {
                libraries.Add(dir);
                
                // Also check architecture-specific subdirectories
                var archDirs = new[]
                {
                    Path.Combine(dir, "x64"),
                    Path.Combine(dir, "x86"),
                    Path.Combine(dir, "arm64"),
                    Path.Combine(dir, "arm")
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
    
    public virtual async Task<SDKResult<List<string>>> LocateBinariesAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var binaries = new List<string>();
        
        // Common binary directories
        var binDirs = new[]
        {
            Path.Combine(path, "Bin"),
            Path.Combine(path, "bin"),
            Path.Combine(path, "Binary"),
            Path.Combine(path, "binary"),
            Path.Combine(path, "binaries")
        };
        
        foreach (var dir in binDirs)
        {
            if (Directory.Exists(dir))
            {
                binaries.Add(dir);
                
                // Also check architecture-specific subdirectories
                var archDirs = new[]
                {
                    Path.Combine(dir, "x64"),
                    Path.Combine(dir, "x86"),
                    Path.Combine(dir, "arm64"),
                    Path.Combine(dir, "arm")
                };
                
                foreach (var archDir in archDirs)
                {
                    if (Directory.Exists(archDir))
                    {
                        binaries.Add(archDir);
                    }
                }
            }
        }
        
        result.Success = binaries.Count > 0;
        result.Value = binaries;
        return await Task.FromResult(result);
    }
    
    public virtual async Task<SDKResult<List<string>>> LocateToolsAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var tools = new List<string>();
        
        // Search in binary directories
        var binResult = await LocateBinariesAsync(path);
        if (binResult.Success && binResult.Value != null)
        {
            foreach (var binDir in binResult.Value)
            {
                if (Directory.Exists(binDir))
                {
                    var files = Directory.GetFiles(binDir, "*.exe", SearchOption.TopDirectoryOnly);
                    tools.AddRange(files);
                    
                    // Also search for executables without extension on Unix
                    if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux) || 
                        RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
                    {
                        var unixFiles = Directory.GetFiles(binDir, "*", SearchOption.TopDirectoryOnly)
                            .Where(f => !f.Contains('.'));
                        tools.AddRange(unixFiles);
                    }
                }
            }
        }
        
        result.Success = tools.Count > 0;
        result.Value = tools;
        return await Task.FromResult(result);
    }
    
    public virtual async Task<SDKResult<string>> GetVersionAsync(string path)
    {
        // Try to extract version from path
        var dirName = new DirectoryInfo(path).Name;
        if (System.Version.TryParse(dirName, out var version))
        {
            return await Task.FromResult(SDKResult<string>.Ok(dirName));
        }
        
        // Try to find version file
        var versionFiles = new[]
        {
            Path.Combine(path, "version.txt"),
            Path.Combine(path, "VERSION"),
            Path.Combine(path, ".version"),
            Path.Combine(path, "config", "version.txt")
        };
        
        foreach (var versionFile in versionFiles)
        {
            if (File.Exists(versionFile))
            {
                try
                {
                    var versionContent = await File.ReadAllTextAsync(versionFile);
                    var trimmedVersion = versionContent.Trim();
                    if (!string.IsNullOrEmpty(trimmedVersion))
                    {
                        return SDKResult<string>.Ok(trimmedVersion);
                    }
                }
                catch
                {
                    // Ignore file read errors
                }
            }
        }
        
        return await Task.FromResult(SDKResult<string>.Ok("Unknown"));
    }
    
    public virtual async Task<bool> IsInstalledAsync()
    {
        var result = await DetectAsync();
        return result.Success && result.Value != null;
    }
    
    /// <summary>
    /// Searches for a file recursively in a directory.
    /// </summary>
    protected bool FindFileInPath(string path, string fileName)
    {
        try
        {
            var files = Directory.GetFiles(path, fileName, SearchOption.AllDirectories);
            return files.Length > 0;
        }
        catch
        {
            return false;
        }
    }
    
    /// <summary>
    /// Gets the current platform.
    /// </summary>
    protected TargetPlatform GetCurrentPlatform()
    {
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            return TargetPlatform.Windows;
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            return TargetPlatform.Linux;
        if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
            return TargetPlatform.macOS;
        
        return TargetPlatform.Unknown;
    }
    
    /// <summary>
    /// Gets the current architecture.
    /// </summary>
    protected TargetArchitecture GetCurrentArchitecture()
    {
        var arch = RuntimeInformation.OSArchitecture;
        return arch switch
        {
            Architecture.X64 => TargetArchitecture.x64,
            Architecture.X86 => TargetArchitecture.x86,
            Architecture.Arm64 => TargetArchitecture.ARM64,
            Architecture.Arm => TargetArchitecture.ARM,
            _ => TargetArchitecture.Unknown
        };
    }
}
