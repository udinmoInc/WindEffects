using Serilog;
using System.Runtime.InteropServices;

namespace IgniteBT.SDK;

/// <summary>
/// Scans for SDKs using Windows Registry.
/// </summary>
public class RegistryScanner
{
    /// <summary>
    /// Scans for the SDK in the Windows Registry.
    /// </summary>
    public async Task<SDKScanResult> ScanAsync(ISDKProvider provider)
    {
        var result = new SDKScanResult();
        
        if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            return await Task.FromResult(result);
        }
        
        // Get registry keys to check for this SDK
        var registryKeys = GetRegistryKeys(provider.SDKName);
        
        foreach (var (hive, keyPath, valueName) in registryKeys)
        {
            try
            {
                using var hiveKey = GetHiveKey(hive);
                if (hiveKey == null) continue;
                
                using var key = hiveKey.OpenSubKey(keyPath);
                if (key == null) continue;
                
                var value = key.GetValue(valueName) as string;
                if (!string.IsNullOrEmpty(value))
                {
                    result.SearchLocations.Add(value);
                    
                    if (Directory.Exists(value))
                    {
                        result.Success = true;
                        result.Value = value;
                        Log.Debug("Found {SDKName} in registry: {Path}", provider.SDKName, value);
                        return result;
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Debug(ex, "Failed to check registry key: {Key}", keyPath);
            }
        }
        
        return await Task.FromResult(result);
    }
    
    /// <summary>
    /// Gets the registry keys to check for a given SDK.
    /// </summary>
    private List<(RegistryHive, string, string?)> GetRegistryKeys(string sdkName)
    {
        return sdkName.ToLowerInvariant() switch
        {
            "vulkan" or "vulkansdk" => new List<(RegistryHive, string, string?)>
            {
                (RegistryHive.LocalMachine, @"SOFTWARE\Khronos\VulkanSDK", null),
                (RegistryHive.LocalMachine, @"SOFTWARE\Khronos\VulkanSDK", "Path"),
                (RegistryHive.CurrentUser, @"SOFTWARE\Khronos\VulkanSDK", null)
            },
            "directx" or "directxsdk" => new List<(RegistryHive, string, string?)>
            {
                (RegistryHive.LocalMachine, @"SOFTWARE\Microsoft\DirectX", "SDKPath"),
                (RegistryHive.LocalMachine, @"SOFTWARE\Wow6432Node\Microsoft\DirectX", "SDKPath")
            },
            "openxr" or "openxrsdk" => new List<(RegistryHive, string, string?)>
            {
                (RegistryHive.LocalMachine, @"SOFTWARE\Khronos\OpenXR", "Path"),
                (RegistryHive.LocalMachine, @"SOFTWARE\Khronos\OpenXR\1.0", "Path")
            },
            "android" or "androidsdk" => new List<(RegistryHive, string, string?)>
            {
                (RegistryHive.LocalMachine, @"SOFTWARE\Android SDK", "Path"),
                (RegistryHive.LocalMachine, @"SOFTWARE\Wow6432Node\Android SDK", "Path")
            },
            "python" or "python3" => new List<(RegistryHive, string, string?)>
            {
                (RegistryHive.LocalMachine, @"SOFTWARE\Python\PythonCore", null),
                (RegistryHive.LocalMachine, @"SOFTWARE\Python\PythonCore\3.11", "InstallPath"),
                (RegistryHive.LocalMachine, @"SOFTWARE\Python\PythonCore\3.12", "InstallPath"),
                (RegistryHive.CurrentUser, @"SOFTWARE\Python\PythonCore", null)
            },
            "dotnet" or ".net" or "dotnetsdk" => new List<(RegistryHive, string, string?)>
            {
                (RegistryHive.LocalMachine, @"SOFTWARE\dotnet\Setup\InstalledVersions\x64", null),
                (RegistryHive.LocalMachine, @"SOFTWARE\Microsoft\dotnet\Setup\InstalledVersions\x64", null)
            },
            "windows" or "windowssdk" => new List<(RegistryHive, string, string?)>
            {
                (RegistryHive.LocalMachine, @"SOFTWARE\Microsoft\Windows Kits\Installed Roots", "KitsRoot10"),
                (RegistryHive.LocalMachine, @"SOFTWARE\Microsoft\Windows Kits\Installed Roots", "KitsRoot81")
            },
            _ => new List<(RegistryHive, string, string?)>()
        };
    }
    
    /// <summary>
    /// Gets the registry hive key.
    /// </summary>
    private Microsoft.Win32.RegistryKey? GetHiveKey(RegistryHive hive)
    {
        if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            return null;
        }
        
        return hive switch
        {
            RegistryHive.LocalMachine => Microsoft.Win32.Registry.LocalMachine,
            RegistryHive.CurrentUser => Microsoft.Win32.Registry.CurrentUser,
            _ => null
        };
    }
    
    /// <summary>
    /// Registry hive enumeration.
    /// </summary>
    private enum RegistryHive
    {
        LocalMachine,
        CurrentUser
    }
}
