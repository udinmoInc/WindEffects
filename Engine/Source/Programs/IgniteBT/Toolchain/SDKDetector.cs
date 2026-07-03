using Serilog;
using System.Runtime.InteropServices;

namespace IgniteBT.Toolchain;

/// <summary>
/// Detects external SDKs like Vulkan SDK, Windows SDK, etc.
/// </summary>
public static class SDKDetector
{
    public class DetectedSDK
    {
        public string Name { get; set; } = string.Empty;
        public string Path { get; set; } = string.Empty;
        public string Version { get; set; } = string.Empty;
        public List<string> IncludePaths { get; set; } = new();
        public List<string> LibraryPaths { get; set; } = new();
    }

    /// <summary>
    /// Detects all available SDKs.
    /// </summary>
    public static List<DetectedSDK> DetectAllSDKs(SDKConfig? config = null)
    {
        var sdks = new List<DetectedSDK>();

        Log.Information("Detecting external SDKs...");

        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            var vulkanSDK = DetectVulkanSDKWindows(config);
            if (vulkanSDK != null)
            {
                sdks.Add(vulkanSDK);
            }
        }
        else if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
        {
            var vulkanSDK = DetectVulkanSDKLinux(config);
            if (vulkanSDK != null)
            {
                sdks.Add(vulkanSDK);
            }
        }
        else if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
        {
            var vulkanSDK = DetectVulkanSDKMac(config);
            if (vulkanSDK != null)
            {
                sdks.Add(vulkanSDK);
            }
        }

        Log.Information("Detected {Count} SDKs", sdks.Count);
        foreach (var sdk in sdks)
        {
            Log.Information("  - {Name} v{Version} at {Path}", sdk.Name, sdk.Version, sdk.Path);
        }

        return sdks;
    }

    /// <summary>
    /// Detects Vulkan SDK on Windows.
    /// </summary>
    private static DetectedSDK? DetectVulkanSDKWindows(SDKConfig? config)
    {
        Log.Information("Detecting Vulkan SDK on Windows");

        // Check SDK config first
        if (config != null)
        {
            var configPath = config.GetSDKPath("VulkanSDK");
            if (!string.IsNullOrEmpty(configPath) && Directory.Exists(configPath))
            {
                Log.Information("Found Vulkan SDK via config: {Path}", configPath);
                return CreateVulkanSDK(configPath);
            }
        }

        // Check VULKAN_SDK environment variable
        var vulkanSdkPath = Environment.GetEnvironmentVariable("VULKAN_SDK");
        Log.Debug("VULKAN_SDK environment variable: {Path}", vulkanSdkPath ?? "(not set)");
        if (!string.IsNullOrEmpty(vulkanSdkPath) && Directory.Exists(vulkanSdkPath))
        {
            Log.Information("Found Vulkan SDK via VULKAN_SDK: {Path}", vulkanSdkPath);
            return CreateVulkanSDK(vulkanSdkPath);
        }

        // Check common installation paths
        var possiblePaths = new[]
        {
            @"C:\VulkanSDK",
            @"C:\VulkanSDK\1.3.290.0",
            @"C:\VulkanSDK\1.3.280.0",
            @"C:\VulkanSDK\1.3.275.0",
            @"C:\VulkanSDK\1.3.268.0",
            @"C:\VulkanSDK\1.3.261.1",
            @"C:\Program Files\VulkanSDK",
            Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86) + @"\VulkanSDK",
            Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles) + @"\VulkanSDK"
        };

        foreach (var path in possiblePaths)
        {
            Log.Debug("Checking path: {Path}", path);
            if (Directory.Exists(path))
            {
                Log.Information("Found Vulkan SDK at: {Path}", path);
                return CreateVulkanSDK(path);
            }
        }

        // Try to find via registry (Windows only)
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            try
            {
                Log.Debug("Checking registry for Vulkan SDK");
                using var key = Microsoft.Win32.Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Khronos\VulkanSDK");
                if (key != null)
                {
                    var sdkPath = key.GetValue(null) as string;
                    Log.Debug("Registry value: {Path}", sdkPath ?? "(null)");
                    if (!string.IsNullOrEmpty(sdkPath) && Directory.Exists(sdkPath))
                    {
                        Log.Information("Found Vulkan SDK via registry: {Path}", sdkPath);
                        return CreateVulkanSDK(sdkPath);
                    }
                }
                else
                {
                    Log.Debug("Registry key not found");
                }
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Failed to check registry for Vulkan SDK");
            }
        }

        Log.Warning("Vulkan SDK not found");
        return null;
    }

    /// <summary>
    /// Detects Vulkan SDK on Linux.
    /// </summary>
    private static DetectedSDK? DetectVulkanSDKLinux(SDKConfig? config)
    {
        Log.Information("Detecting Vulkan SDK on Linux");

        // Check SDK config first
        if (config != null)
        {
            var configPath = config.GetSDKPath("VulkanSDK");
            if (!string.IsNullOrEmpty(configPath) && Directory.Exists(configPath))
            {
                Log.Information("Found Vulkan SDK via config: {Path}", configPath);
                return CreateVulkanSDK(configPath);
            }
        }

        // Check VULKAN_SDK environment variable
        var vulkanSdkPath = Environment.GetEnvironmentVariable("VULKAN_SDK");
        if (!string.IsNullOrEmpty(vulkanSdkPath) && Directory.Exists(vulkanSdkPath))
        {
            Log.Information("Found Vulkan SDK via VULKAN_SDK: {Path}", vulkanSdkPath);
            return CreateVulkanSDK(vulkanSdkPath);
        }

        // Check common installation paths
        var possiblePaths = new[]
        {
            "/usr/local/share/vulkan",
            "/usr/share/vulkan",
            "/opt/vulkan",
            "/usr/local/vulkan"
        };

        foreach (var path in possiblePaths)
        {
            if (Directory.Exists(path))
            {
                Log.Information("Found Vulkan SDK at: {Path}", path);
                return CreateVulkanSDK(path);
            }
        }

        Log.Warning("Vulkan SDK not found");
        return null;
    }

    /// <summary>
    /// Detects Vulkan SDK on macOS.
    /// </summary>
    private static DetectedSDK? DetectVulkanSDKMac(SDKConfig? config)
    {
        Log.Information("Detecting Vulkan SDK on macOS");

        // Check SDK config first
        if (config != null)
        {
            var configPath = config.GetSDKPath("VulkanSDK");
            if (!string.IsNullOrEmpty(configPath) && Directory.Exists(configPath))
            {
                Log.Information("Found Vulkan SDK via config: {Path}", configPath);
                return CreateVulkanSDK(configPath);
            }
        }

        // Check VULKAN_SDK environment variable
        var vulkanSdkPath = Environment.GetEnvironmentVariable("VULKAN_SDK");
        if (!string.IsNullOrEmpty(vulkanSdkPath) && Directory.Exists(vulkanSdkPath))
        {
            Log.Information("Found Vulkan SDK via VULKAN_SDK: {Path}", vulkanSdkPath);
            return CreateVulkanSDK(vulkanSdkPath);
        }

        // Check common installation paths
        var possiblePaths = new[]
        {
            "/usr/local/share/vulkan",
            "/opt/vulkan",
            "/usr/local/vulkan",
            "/usr/local/vulkansdk"
        };

        foreach (var path in possiblePaths)
        {
            if (Directory.Exists(path))
            {
                Log.Information("Found Vulkan SDK at: {Path}", path);
                return CreateVulkanSDK(path);
            }
        }

        Log.Warning("Vulkan SDK not found");
        return null;
    }

    /// <summary>
    /// Creates a DetectedSDK object for Vulkan SDK.
    /// </summary>
    private static DetectedSDK CreateVulkanSDK(string path)
    {
        var sdk = new DetectedSDK
        {
            Name = "VulkanSDK",
            Path = path,
            Version = ExtractVulkanVersion(path)
        };

        // Add include paths
        var includePath = Path.Combine(path, "Include");
        if (Directory.Exists(includePath))
        {
            sdk.IncludePaths.Add(includePath);
            
            // Also add vulkan subdirectory if it exists
            var vulkanIncludePath = Path.Combine(includePath, "vulkan");
            if (Directory.Exists(vulkanIncludePath))
            {
                sdk.IncludePaths.Add(vulkanIncludePath);
            }
        }

        // Add library paths
        var libPath = Path.Combine(path, "Lib");
        if (Directory.Exists(libPath))
        {
            sdk.LibraryPaths.Add(libPath);
        }

        return sdk;
    }

    /// <summary>
    /// Extracts Vulkan SDK version from path or configuration file.
    /// </summary>
    private static string ExtractVulkanVersion(string path)
    {
        // Try to extract version from path (e.g., "C:\VulkanSDK\1.3.290.0")
        var dirName = new DirectoryInfo(path).Name;
        if (System.Version.TryParse(dirName, out var version))
        {
            return dirName;
        }

        // Try to read from configuration file
        var configFile = Path.Combine(path, "config", "vk_sdk_config.json");
        if (File.Exists(configFile))
        {
            try
            {
                var configContent = File.ReadAllText(configFile);
                // Simple parsing - look for version pattern
                var match = System.Text.RegularExpressions.Regex.Match(configContent, @"""version""\s*:\s*""([^""]+)""");
                if (match.Success)
                {
                    return match.Groups[1].Value;
                }
            }
            catch
            {
                // Ignore parsing errors
            }
        }

        return "Unknown";
    }
}
