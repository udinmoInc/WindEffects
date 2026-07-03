using Serilog;
using System.Runtime.InteropServices;

namespace IgniteBT.SDK.Providers;

/// <summary>
/// Provider for Vulkan SDK.
/// </summary>
public class VulkanSDKProvider : BaseSDKProvider
{
    public override string SDKName => "VulkanSDK";
    public override int Priority => 50;
    
    public override List<TargetPlatform> SupportedPlatforms => new()
    {
        TargetPlatform.Windows,
        TargetPlatform.Linux,
        TargetPlatform.macOS
    };
    
    public override List<TargetArchitecture> SupportedArchitectures => new()
    {
        TargetArchitecture.x64,
        TargetArchitecture.ARM64
    };
    
    protected override List<string> EnvironmentVariableNames => new()
    {
        "VULKAN_SDK",
        "VK_SDK_PATH"
    };
    
    protected override List<string> DefaultPaths => new()
    {
        @"C:\VulkanSDK",
        @"C:\VulkanSDK\1.3.290.0",
        @"C:\VulkanSDK\1.3.280.0",
        @"C:\VulkanSDK\1.3.275.0",
        @"C:\VulkanSDK\1.3.268.0",
        @"C:\VulkanSDK\1.3.261.1",
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "VulkanSDK"),
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "VulkanSDK"),
        "/usr/local/share/vulkan",
        "/usr/share/vulkan",
        "/opt/vulkan",
        "/usr/local/vulkan"
    };
    
    protected override List<string> ExpectedHeaders => new()
    {
        "volk.h",
        "vulkan/vulkan.h",
        "vulkan/vulkan_core.h"
    };
    
    protected override List<string> ExpectedLibraries => new()
    {
        "vulkan-1.lib",
        "vulkan.lib",
        "libvulkan.so",
        "libvulkan.dylib"
    };
    
    protected override List<string> ExpectedTools => new()
    {
        "glslangValidator.exe",
        "glslangValidator",
        "spirv-val.exe",
        "spirv-val"
    };
    
    public override async Task<SDKResult<List<string>>> LocateHeadersAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var headers = new List<string>();
        
        // Vulkan SDK has specific include structure
        var includePaths = new[]
        {
            Path.Combine(path, "Include"),
            Path.Combine(path, "include"),
            Path.Combine(path, "Include", "vulkan"),
            Path.Combine(path, "include", "vulkan")
        };
        
        foreach (var includePath in includePaths)
        {
            if (Directory.Exists(includePath))
            {
                headers.Add(includePath);
                Log.Debug("Found Vulkan include path: {Path}", includePath);
            }
        }
        
        // Also add the parent include directory if it contains vulk.h
        if (Directory.Exists(path))
        {
            var volkPath = Path.Combine(path, "volk.h");
            if (File.Exists(volkPath))
            {
                headers.Add(path);
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
        
        // Vulkan SDK library structure
        var libPaths = new[]
        {
            Path.Combine(path, "Lib"),
            Path.Combine(path, "lib"),
            Path.Combine(path, "Library")
        };
        
        foreach (var libPath in libPaths)
        {
            if (Directory.Exists(libPath))
            {
                libraries.Add(libPath);
                
                // Add architecture-specific subdirectories
                var arch = GetCurrentArchitecture();
                var archPath = arch switch
                {
                    TargetArchitecture.x64 => Path.Combine(libPath, "x64"),
                    TargetArchitecture.ARM64 => Path.Combine(libPath, "arm64"),
                    _ => null
                };
                
                if (archPath != null && Directory.Exists(archPath))
                {
                    libraries.Add(archPath);
                }
            }
        }
        
        result.Success = libraries.Count > 0;
        result.Value = libraries;
        return await Task.FromResult(result);
    }
    
    public override async Task<SDKResult<string>> GetVersionAsync(string path)
    {
        // Try to extract version from path
        var dirName = new DirectoryInfo(path).Name;
        if (System.Version.TryParse(dirName, out _))
        {
            return await Task.FromResult(SDKResult<string>.Ok(dirName));
        }
        
        // Try to read from vk_sdk_config.json
        var configPath = Path.Combine(path, "config", "vk_sdk_config.json");
        if (File.Exists(configPath))
        {
            try
            {
                var configContent = await File.ReadAllTextAsync(configPath);
                // Simple regex to extract version
                var match = System.Text.RegularExpressions.Regex.Match(
                    configContent, @"""version""\s*:\s*""([^""]+)""");
                if (match.Success)
                {
                    return SDKResult<string>.Ok(match.Groups[1].Value);
                }
            }
            catch
            {
                // Ignore parse errors
            }
        }
        
        // Try to find version.txt
        var versionFile = Path.Combine(path, "version.txt");
        if (File.Exists(versionFile))
        {
            try
            {
                var versionContent = await File.ReadAllTextAsync(versionFile);
                var trimmed = versionContent.Trim();
                if (!string.IsNullOrEmpty(trimmed))
                {
                    return SDKResult<string>.Ok(trimmed);
                }
            }
            catch
            {
                // Ignore file read errors
            }
        }
        
        return await Task.FromResult(SDKResult<string>.Ok("Unknown"));
    }
}
