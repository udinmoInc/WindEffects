using Serilog;
using System.Runtime.InteropServices;

namespace IgniteBT.SDK.Providers;

/// <summary>
/// Provider for Python SDK.
/// </summary>
public class PythonProvider : BaseSDKProvider
{
    public override string SDKName => "Python";
    public override int Priority => 55;
    
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
        "PYTHON_PATH",
        "PYTHONHOME",
        "PYTHON3_PATH"
    };
    
    protected override List<string> DefaultPaths => new()
    {
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Python311"),
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Python312"),
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Python310"),
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Python311"),
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Python312"),
        "/usr/local/bin/python3",
        "/usr/bin/python3",
        "/opt/python3",
        "/usr/local/python3"
    };
    
    protected override List<string> ExpectedTools => new()
    {
        "python.exe",
        "python3.exe",
        "python",
        "python3",
        "pip.exe",
        "pip",
        "pip3"
    };
    
    public override async Task<SDKResult<List<string>>> LocateHeadersAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var headers = new List<string>();
        
        // Python SDK structure
        var includePaths = new[]
        {
            Path.Combine(path, "include"),
            Path.Combine(path, "Include")
        };
        
        foreach (var includePath in includePaths)
        {
            if (Directory.Exists(includePath))
            {
                headers.Add(includePath);
                
                // Also check for version-specific include directory
                var versionDirs = Directory.GetDirectories(includePath);
                foreach (var versionDir in versionDirs)
                {
                    headers.Add(versionDir);
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
        
        // Python SDK library structure
        var libPaths = new[]
        {
            Path.Combine(path, "libs"),
            Path.Combine(path, "Lib"),
            Path.Combine(path, "lib"),
            Path.Combine(path, "Libs")
        };
        
        foreach (var libPath in libPaths)
        {
            if (Directory.Exists(libPath))
            {
                libraries.Add(libPath);
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
        
        // Python executables are in the root directory
        var pythonExes = new[]
        {
            "python.exe",
            "python3.exe",
            "python",
            "python3"
        };
        
        foreach (var exe in pythonExes)
        {
            var exePath = Path.Combine(path, exe);
            if (File.Exists(exePath))
            {
                tools.Add(exePath);
            }
        }
        
        // Also add Scripts directory on Windows
        var scriptsPath = Path.Combine(path, "Scripts");
        if (Directory.Exists(scriptsPath))
        {
            tools.Add(scriptsPath);
            
            var pipExes = new[]
            {
                "pip.exe",
                "pip3.exe",
                "pip",
                "pip3"
            };
            
            foreach (var pipExe in pipExes)
            {
                var pipPath = Path.Combine(scriptsPath, pipExe);
                if (File.Exists(pipPath))
                {
                    tools.Add(pipPath);
                }
            }
        }
        
        result.Success = tools.Count > 0;
        result.Value = tools;
        return await Task.FromResult(result);
    }
    
    public override async Task<SDKResult<string>> GetVersionAsync(string path)
    {
        try
        {
            var pythonExe = RuntimeInformation.IsOSPlatform(OSPlatform.Windows) 
                ? "python.exe" 
                : "python3";
            
            var pythonPath = Path.Combine(path, pythonExe);
            if (!File.Exists(pythonPath))
            {
                // Try python without version
                pythonExe = RuntimeInformation.IsOSPlatform(OSPlatform.Windows) 
                    ? "python.exe" 
                    : "python";
                pythonPath = Path.Combine(path, pythonExe);
            }
            
            if (!File.Exists(pythonPath))
            {
                return await Task.FromResult(SDKResult<string>.Ok("Unknown"));
            }
            
            var startInfo = new System.Diagnostics.ProcessStartInfo
            {
                FileName = pythonPath,
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
                
                // Python outputs "Python 3.11.0" - extract version
                var version = output.Replace("Python", "").Trim();
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
