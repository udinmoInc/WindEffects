using Serilog;

namespace IgniteBT.Workspace.SDK;

/// <summary>
/// Scans for SDKs using environment variables.
/// </summary>
public class EnvironmentScanner
{
    /// <summary>
    /// Scans for the SDK using environment variables.
    /// </summary>
    public async Task<SDKScanResult> ScanAsync(ISDKProvider provider)
    {
        var result = new SDKScanResult();
        
        // Get environment variable names for this SDK
        var envVarNames = GetEnvironmentVariableNames(provider.SDKName);
        
        foreach (var envVarName in envVarNames)
        {
            var envValue = Environment.GetEnvironmentVariable(envVarName);
            if (!string.IsNullOrEmpty(envValue))
            {
                result.SearchLocations.Add(envValue);
                
                if (Directory.Exists(envValue))
                {
                    result.Success = true;
                    result.Value = envValue;
                    Log.Debug("Found {SDKName} via environment variable {Var}: {Path}", 
                        provider.SDKName, envVarName, envValue);
                    return result;
                }
            }
        }
        
        return await Task.FromResult(result);
    }
    
    /// <summary>
    /// Gets the environment variable names to check for a given SDK.
    /// </summary>
    private List<string> GetEnvironmentVariableNames(string sdkName)
    {
        return sdkName.ToLowerInvariant() switch
        {
            "vulkan" or "vulkansdk" => new List<string> { "VULKAN_SDK", "VK_SDK_PATH" },
            "directx" or "directxsdk" => new List<string> { "DXSDK_DIR", "DIRECTX_SDK" },
            "openxr" or "openxrsdk" => new List<string> { "OPENXR_SDK" },
            "android" or "androidsdk" => new List<string> { "ANDROID_SDK_ROOT", "ANDROID_HOME" },
            "python" or "python3" => new List<string> { "PYTHON_PATH", "PYTHONHOME" },
            "dotnet" or ".net" or "dotnetsdk" => new List<string> { "DOTNET_ROOT", "DOTNET_INSTALL_DIR" },
            "windows" or "windowssdk" => new List<string> { "WindowsSdkDir", "WINDOWSSDKDIR" },
            _ => new List<string>()
        };
    }
}
