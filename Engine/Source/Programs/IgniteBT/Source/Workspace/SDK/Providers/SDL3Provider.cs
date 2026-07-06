namespace IgniteBT.Workspace.SDK.Providers;

/// <summary>
/// Provider for SDL3 (Simple DirectMedia Layer).
/// </summary>
public class SDL3Provider : BaseSDKProvider
{
    public override string SDKName => "SDL3";
    public override int Priority => 40;

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
        "SDL3_PATH",
        "SDL_PATH"
    };

    protected override List<string> ExpectedHeaders => new()
    {
        "SDL3/SDL.h"
    };

    protected override List<string> ExpectedLibraries => new()
    {
        "SDL3.lib",
        "SDL3-static.lib",
        "libSDL3.a",
        "libSDL3.dylib"
    };

    public override async Task<SDKResult<List<string>>> LocateHeadersAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var headers = new List<string>();

        var includePaths = new[]
        {
            Path.Combine(path, "include"),
            Path.Combine(path, "Include"),
            path
        };

        foreach (var includePath in includePaths)
        {
            if (Directory.Exists(includePath) &&
                File.Exists(Path.Combine(includePath, "SDL3", "SDL.h")))
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

        var libDirs = new[]
        {
            Path.Combine(path, "build-shared", "Release"),
            Path.Combine(path, "build", "Release"),
            Path.Combine(path, "lib", "x64"),
            Path.Combine(path, "lib"),
            Path.Combine(path, "Lib", "x64"),
            Path.Combine(path, "Lib"),
            Path.Combine(path, "build")
        };

        foreach (var dir in libDirs)
        {
            if (Directory.Exists(dir))
            {
                libraries.Add(dir);
            }
        }

        result.Success = libraries.Count > 0;
        result.Value = libraries;
        return await Task.FromResult(result);
    }

    public override async Task<SDKResult<List<string>>> LocateBinariesAsync(string path)
    {
        var result = new SDKResult<List<string>>();
        var binaries = new List<string>();

        var runtimeDirs = new[]
        {
            Path.Combine(path, "build-shared", "Release"),
            Path.Combine(path, "build-shared", "Debug"),
            Path.Combine(path, "build", "Release"),
            Path.Combine(path, "build", "Debug"),
            Path.Combine(path, "bin"),
            Path.Combine(path, "Bin")
        };

        foreach (var dir in runtimeDirs)
        {
            if (Directory.Exists(dir))
            {
                binaries.Add(dir);
            }
        }

        var baseResult = await base.LocateBinariesAsync(path);
        if (baseResult.Success && baseResult.Value != null)
        {
            foreach (var dir in baseResult.Value)
            {
                if (!binaries.Contains(dir))
                {
                    binaries.Add(dir);
                }
            }
        }

        result.Success = binaries.Count > 0;
        result.Value = binaries;
        return result;
    }
}
