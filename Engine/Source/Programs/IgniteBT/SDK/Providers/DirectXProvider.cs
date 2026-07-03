using Serilog;
using System.Runtime.InteropServices;

namespace IgniteBT.SDK.Providers;

/// <summary>
/// Provider for DirectX SDK.
/// </summary>
public class DirectXProvider : BaseSDKProvider
{
    public override string SDKName => "DirectX";
    public override int Priority => 40;
    
    public override List<TargetPlatform> SupportedPlatforms => new()
    {
        TargetPlatform.Windows
    };
    
    public override List<TargetArchitecture> SupportedArchitectures => new()
    {
        TargetArchitecture.x64,
        TargetArchitecture.x86
    };
    
    protected override List<string> EnvironmentVariableNames => new()
    {
        "DXSDK_DIR",
        "DIRECTX_SDK"
    };
    
    protected override List<string> DefaultPaths => new()
    {
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Microsoft DirectX SDK (June 2010)"),
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Microsoft DirectX SDK"),
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Microsoft DirectX SDK"),
        @"C:\DXSDK"
    };
    
    protected override List<string> ExpectedHeaders => new()
    {
        "d3d11.h",
        "d3d12.h",
        "dxgi.h"
    };
    
    protected override List<string> ExpectedLibraries => new()
    {
        "d3d11.lib",
        "d3d12.lib",
        "dxgi.lib"
    };
    
    public override async Task<SDKResult<List<string>>> LocateHeadersAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var headers = new List<string>();
        
        // DirectX SDK structure
        var includePaths = new[]
        {
            Path.Combine(path, "Include"),
            Path.Combine(path, "include")
        };
        
        foreach (var includePath in includePaths)
        {
            if (Directory.Exists(includePath))
            {
                headers.Add(includePath);
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
        
        // DirectX SDK library structure
        var libPaths = new[]
        {
            Path.Combine(path, "Lib"),
            Path.Combine(path, "lib")
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
                    Path.Combine(libPath, "x86")
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
    
    public override async Task<bool> IsInstalledAsync()
    {
        if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            return false;
        }
        
        return await base.IsInstalledAsync();
    }
}
