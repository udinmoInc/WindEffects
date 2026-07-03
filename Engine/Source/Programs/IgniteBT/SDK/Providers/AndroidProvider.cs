using Serilog;
using System.Runtime.InteropServices;

namespace IgniteBT.SDK.Providers;

/// <summary>
/// Provider for Android SDK.
/// </summary>
public class AndroidProvider : BaseSDKProvider
{
    public override string SDKName => "Android";
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
        "ANDROID_SDK_ROOT",
        "ANDROID_HOME",
        "ANDROID_SDK"
    };
    
    protected override List<string> DefaultPaths => new()
    {
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Android", "Sdk"),
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "AppData", "Local", "Android", "Sdk"),
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "Android", "Sdk"),
        "/usr/local/android-sdk",
        "/opt/android-sdk",
        "/home/Android/Sdk"
    };
    
    protected override List<string> ExpectedTools => new()
    {
        "adb.exe",
        "adb",
        "emulator.exe",
        "emulator",
        "avdmanager.exe",
        "avdmanager",
        "sdkmanager.exe",
        "sdkmanager"
    };
    
    public override async Task<SDKResult<List<string>>> LocateHeadersAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var headers = new List<string>();
        
        // Android SDK headers are in platforms/android-<version>/include
        var platformsPath = Path.Combine(path, "platforms");
        if (Directory.Exists(platformsPath))
        {
            var platformDirs = Directory.GetDirectories(platformsPath);
            foreach (var platformDir in platformDirs)
            {
                var includePath = Path.Combine(platformDir, "include");
                if (Directory.Exists(includePath))
                {
                    headers.Add(includePath);
                }
            }
        }
        
        // Also check NDK headers if present
        var ndkPath = Path.Combine(path, "ndk");
        if (Directory.Exists(ndkPath))
        {
            var ndkDirs = Directory.GetDirectories(ndkPath);
            foreach (var ndkDir in ndkDirs)
            {
                var ndkIncludePath = Path.Combine(ndkDir, "include");
                if (Directory.Exists(ndkIncludePath))
                {
                    headers.Add(ndkIncludePath);
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
        
        // Android SDK libraries are in platforms/android-<version>/arch
        var platformsPath = Path.Combine(path, "platforms");
        if (Directory.Exists(platformsPath))
        {
            var platformDirs = Directory.GetDirectories(platformsPath);
            foreach (var platformDir in platformDirs)
            {
                var libPath = Path.Combine(platformDir, "lib");
                if (Directory.Exists(libPath))
                {
                    libraries.Add(libPath);
                    
                    // Add architecture-specific subdirectories
                    var archDirs = Directory.GetDirectories(libPath);
                    foreach (var archDir in archDirs)
                    {
                        libraries.Add(archDir);
                    }
                }
            }
        }
        
        // Also check NDK libraries if present
        var ndkPath = Path.Combine(path, "ndk");
        if (Directory.Exists(ndkPath))
        {
            var ndkDirs = Directory.GetDirectories(ndkPath);
            foreach (var ndkDir in ndkDirs)
            {
                var ndkLibPath = Path.Combine(ndkDir, "lib");
                if (Directory.Exists(ndkLibPath))
                {
                    libraries.Add(ndkLibPath);
                    
                    // Add architecture-specific subdirectories
                    var archDirs = Directory.GetDirectories(ndkLibPath);
                    foreach (var archDir in archDirs)
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
    
    public override async Task<SDKResult<List<string>>> LocateToolsAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var tools = new List<string>();
        
        // Android SDK tools are in platform-tools, tools, and build-tools
        var toolDirs = new[]
        {
            Path.Combine(path, "platform-tools"),
            Path.Combine(path, "tools"),
            Path.Combine(path, "build-tools")
        };
        
        foreach (var toolDir in toolDirs)
        {
            if (Directory.Exists(toolDir))
            {
                tools.Add(toolDir);
                
                // Also check version-specific build-tools directories
                if (toolDir.EndsWith("build-tools"))
                {
                    var versionDirs = Directory.GetDirectories(toolDir);
                    foreach (var versionDir in versionDirs)
                    {
                        tools.Add(versionDir);
                    }
                }
            }
        }
        
        result.Success = tools.Count > 0;
        result.Value = tools;
        return await Task.FromResult(result);
    }
    
    public override async Task<SDKResult<string>> GetVersionAsync(string path)
    {
        // Try to read from source.properties file
        var sourcePropsPath = Path.Combine(path, "source.properties");
        if (File.Exists(sourcePropsPath))
        {
            try
            {
                var content = await File.ReadAllTextAsync(sourcePropsPath);
                var match = System.Text.RegularExpressions.Regex.Match(
                    content, @"Pkg\.Revision\s*=\s*(\d+\.?\d*\.?\d*)");
                if (match.Success)
                {
                    return SDKResult<string>.Ok(match.Groups[1].Value);
                }
            }
            catch
            {
                // Ignore errors
            }
        }
        
        // Try to extract from build-tools version directories
        var buildToolsPath = Path.Combine(path, "build-tools");
        if (Directory.Exists(buildToolsPath))
        {
            var versionDirs = Directory.GetDirectories(buildToolsPath);
            if (versionDirs.Length > 0)
            {
                var latestVersion = new DirectoryInfo(versionDirs.OrderByDescending(d => d).First()).Name;
                return SDKResult<string>.Ok(latestVersion);
            }
        }
        
        return await Task.FromResult(SDKResult<string>.Ok("Unknown"));
    }
}
