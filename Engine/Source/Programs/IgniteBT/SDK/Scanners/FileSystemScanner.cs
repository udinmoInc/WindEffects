using Serilog;
using System.Runtime.InteropServices;

namespace IgniteBT.SDK;

/// <summary>
/// Scans for SDKs in default installation locations.
/// </summary>
public class FileSystemScanner
{
    /// <summary>
    /// Scans for the SDK in default installation locations.
    /// </summary>
    public async Task<SDKScanResult> ScanAsync(ISDKProvider provider)
    {
        var result = new SDKScanResult();
        
        // Get default installation paths for this SDK
        var defaultPaths = GetDefaultPaths(provider.SDKName);
        
        foreach (var path in defaultPaths)
        {
            result.SearchLocations.Add(path);
            
            if (Directory.Exists(path))
            {
                result.Success = true;
                result.Value = path;
                Log.Debug("Found {SDKName} in default location: {Path}", provider.SDKName, path);
                return result;
            }
        }
        
        return await Task.FromResult(result);
    }
    
    /// <summary>
    /// Gets default installation paths for a given SDK.
    /// </summary>
    private List<string> GetDefaultPaths(string sdkName)
    {
        var paths = new List<string>();
        var programFiles = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);
        var programFilesX86 = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86);
        var localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        
        switch (sdkName.ToLowerInvariant())
        {
            case "vulkan":
            case "vulkansdk":
                paths.AddRange(new[]
                {
                    @"C:\VulkanSDK",
                    @"C:\VulkanSDK\1.3.290.0",
                    @"C:\VulkanSDK\1.3.280.0",
                    @"C:\VulkanSDK\1.3.275.0",
                    @"C:\VulkanSDK\1.3.268.0",
                    Path.Combine(programFiles, "VulkanSDK"),
                    Path.Combine(programFilesX86, "VulkanSDK"),
                    "/usr/local/share/vulkan",
                    "/usr/share/vulkan",
                    "/opt/vulkan",
                    "/usr/local/vulkan"
                });
                break;
                
            case "directx":
            case "directxsdk":
                paths.AddRange(new[]
                {
                    Path.Combine(programFilesX86, "Microsoft DirectX SDK (June 2010)"),
                    Path.Combine(programFilesX86, "Microsoft DirectX SDK"),
                    Path.Combine(programFiles, "Microsoft DirectX SDK"),
                    @"C:\DXSDK"
                });
                break;
                
            case "openxr":
            case "openxrsdk":
                paths.AddRange(new[]
                {
                    Path.Combine(programFiles, "OpenXR"),
                    Path.Combine(localAppData, "OpenXR"),
                    "/usr/local/include/openxr",
                    "/usr/include/openxr"
                });
                break;
                
            case "android":
            case "androidsdk":
                paths.AddRange(new[]
                {
                    Path.Combine(localAppData, "Android", "Sdk"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "AppData", "Local", "Android", "Sdk"),
                    "/usr/local/android-sdk",
                    "/opt/android-sdk"
                });
                break;
                
            case "python":
            case "python3":
                paths.AddRange(new[]
                {
                    Path.Combine(programFiles, "Python311"),
                    Path.Combine(programFiles, "Python312"),
                    Path.Combine(programFiles, "Python310"),
                    Path.Combine(programFilesX86, "Python311"),
                    Path.Combine(programFilesX86, "Python312"),
                    "/usr/local/bin/python3",
                    "/usr/bin/python3",
                    "/opt/python3"
                });
                break;
                
            case "dotnet":
            case ".net":
            case "dotnetsdk":
                paths.AddRange(new[]
                {
                    Path.Combine(programFiles, "dotnet"),
                    Path.Combine(localAppData, "Microsoft", "dotnet"),
                    "/usr/local/share/dotnet",
                    "/usr/share/dotnet",
                    "/opt/dotnet"
                });
                break;
                
            case "windows":
            case "windowssdk":
                paths.AddRange(new[]
                {
                    Path.Combine(programFiles, "Windows Kits", "10"),
                    Path.Combine(programFilesX86, "Windows Kits", "10"),
                    Path.Combine(programFiles, "Windows Kits", "8.1"),
                    Path.Combine(programFilesX86, "Windows Kits", "8.1")
                });
                break;
        }
        
        return paths;
    }
}
