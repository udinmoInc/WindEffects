using Serilog;
using System.Runtime.InteropServices;

namespace IgniteBT.Workspace.SDK.Providers;

/// <summary>
/// Provider for .NET SDK.
/// </summary>
public class DotNetProvider : BaseSDKProvider
{
    public override string SDKName => "DotNet";
    public override int Priority => 60;
    
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
        "DOTNET_ROOT",
        "DOTNET_INSTALL_DIR"
    };
    
    protected override List<string> DefaultPaths => new()
    {
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "dotnet"),
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Microsoft", "dotnet"),
        "/usr/local/share/dotnet",
        "/usr/share/dotnet",
        "/opt/dotnet"
    };
    
    protected override List<string> ExpectedTools => new()
    {
        "dotnet.exe",
        "dotnet"
    };
    
    public override async Task<SDKResult<List<string>>> LocateHeadersAsync(string path)
    {
        // .NET SDK doesn't typically have C++ headers
        var result = new SDKResult<List<string>>
        {
            Success = true,
            Value = new List<string>()
        };
        return await Task.FromResult(result);
    }
    
    public override async Task<SDKResult<List<string>>> LocateLibrariesAsync(string path)
    {
        // .NET SDK doesn't typically have native libraries
        var result = new SDKResult<List<string>>
        {
            Success = true,
            Value = new List<string>()
        };
        return await Task.FromResult(result);
    }
    
    public override async Task<SDKResult<List<string>>> LocateToolsAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var tools = new List<string>();
        
        // .NET SDK tools are in the root directory
        var dotnetExe = RuntimeInformation.IsOSPlatform(OSPlatform.Windows) 
            ? "dotnet.exe" 
            : "dotnet";
        
        var dotnetPath = Path.Combine(path, dotnetExe);
        if (File.Exists(dotnetPath))
        {
            tools.Add(dotnetPath);
        }
        
        // Also add the directory itself for PATH
        if (Directory.Exists(path))
        {
            tools.Add(path);
        }
        
        result.Success = tools.Count > 0;
        result.Value = tools;
        return await Task.FromResult(result);
    }
    
    public override async Task<SDKResult<string>> GetVersionAsync(string path)
    {
        try
        {
            var dotnetExe = RuntimeInformation.IsOSPlatform(OSPlatform.Windows) 
                ? "dotnet.exe" 
                : "dotnet";
            
            var dotnetPath = Path.Combine(path, dotnetExe);
            if (!File.Exists(dotnetPath))
            {
                return await Task.FromResult(SDKResult<string>.Ok("Unknown"));
            }
            
            var startInfo = new System.Diagnostics.ProcessStartInfo
            {
                FileName = dotnetPath,
                Arguments = "--version",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };
            
            using var process = System.Diagnostics.Process.Start(startInfo);
            if (process != null)
            {
                var output = await process.StandardOutput.ReadToEndAsync();
                await process.WaitForExitAsync();
                
                var version = output.Trim();
                if (!string.IsNullOrEmpty(version))
                {
                    return SDKResult<string>.Ok(version);
                }
            }
        }
        catch
        {
            // Ignore errors
        }
        
        return await Task.FromResult(SDKResult<string>.Ok("Unknown"));
    }
}
