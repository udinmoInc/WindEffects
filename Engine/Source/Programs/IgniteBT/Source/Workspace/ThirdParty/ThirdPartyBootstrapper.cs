using Serilog;
using System.Diagnostics;

namespace IgniteBT.Workspace.ThirdParty;

/// <summary>
/// Ensures required third-party sources are present before dependency resolution.
/// </summary>
public static class ThirdPartyBootstrapper
{
    private const string Sdl3Remote = "https://github.com/libsdl-org/SDL.git";
    private const string Sdl3Branch = "release-3.2.8";
    private const string GlmRemote = "https://github.com/g-truc/glm.git";
    private const string GlmTag = "1.0.1";

    public static async Task<bool> EnsureRequiredAsync(string engineRoot, int jobs = 0)
    {
        var thirdPartyRoot = Path.Combine(engineRoot, "ThirdParty");
        Directory.CreateDirectory(thirdPartyRoot);

        var glmOk = await EnsureGlmAsync(Path.Combine(thirdPartyRoot, "glm"));
        var sdlOk = await EnsureSdl3Async(Path.Combine(thirdPartyRoot, "SDL3"), jobs);

        if (!glmOk || !sdlOk)
        {
            Log.Error("Third-party bootstrap failed. Install git and cmake, then rerun the build.");
            return false;
        }

        SDK.SDKManager.Instance.ClearCache();
        return true;
    }

    private static async Task<bool> EnsureGlmAsync(string glmPath)
    {
        if (File.Exists(Path.Combine(glmPath, "glm", "glm.hpp")))
        {
            return true;
        }

        Log.Warning("GLM headers missing at {Path}. Cloning {Tag}...", glmPath, GlmTag);
        if (Directory.Exists(glmPath))
        {
            Directory.Delete(glmPath, recursive: true);
        }

        if (!await RunProcessAsync("git", $"clone --depth 1 --branch {GlmTag} {GlmRemote} \"{glmPath}\""))
        {
            return false;
        }

        if (!File.Exists(Path.Combine(glmPath, "glm", "glm.hpp")))
        {
            Log.Error("GLM clone completed but glm/glm.hpp is still missing.");
            return false;
        }

        Log.Information("GLM bootstrapped at {Path}", glmPath);
        return true;
    }

    private static async Task<bool> EnsureSdl3Async(string sdlPath, int jobs)
    {
        var headerPath = Path.Combine(sdlPath, "include", "SDL3", "SDL.h");
        var releaseDll = Path.Combine(sdlPath, "build-shared", "Release", "SDL3.dll");

        if (File.Exists(headerPath) && File.Exists(releaseDll))
        {
            return true;
        }

        if (!File.Exists(headerPath))
        {
            Log.Warning("SDL3 headers missing at {Path}. Cloning {Branch}...", sdlPath, Sdl3Branch);
            if (Directory.Exists(sdlPath))
            {
                Directory.Delete(sdlPath, recursive: true);
            }

            if (!await RunProcessAsync("git", $"clone --depth 1 --branch {Sdl3Branch} {Sdl3Remote} \"{sdlPath}\""))
            {
                return false;
            }
        }

        if (!File.Exists(headerPath))
        {
            Log.Error("SDL3 clone completed but include/SDL3/SDL.h is still missing.");
            return false;
        }

        if (File.Exists(releaseDll))
        {
            Log.Information("SDL3 already built at {Path}", releaseDll);
            return true;
        }

        Log.Information("Building SDL3 shared library (Release)...");
        var buildDir = Path.Combine(sdlPath, "build-shared");
        var configureArgs =
            $"-S \"{sdlPath}\" -B \"{buildDir}\" -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TESTS=OFF -DSDL_TEST=OFF";
        if (!await RunProcessAsync("cmake", configureArgs))
        {
            return false;
        }

        var parallel = jobs > 0 ? jobs : Math.Max(1, Environment.ProcessorCount);
        var buildArgs = $"--build \"{buildDir}\" --config Release --target SDL3-shared -j {parallel}";
        if (!await RunProcessAsync("cmake", buildArgs))
        {
            return false;
        }

        if (!File.Exists(releaseDll))
        {
            Log.Error("SDL3 build finished but {Path} was not produced.", releaseDll);
            return false;
        }

        Log.Information("SDL3 bootstrapped: headers + {Dll}", releaseDll);
        return true;
    }

    private static async Task<bool> RunProcessAsync(string fileName, string arguments)
    {
        Log.Information("Running: {File} {Args}", fileName, arguments);

        using var process = new Process
        {
            StartInfo = new ProcessStartInfo
            {
                FileName = fileName,
                Arguments = arguments,
                UseShellExecute = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                CreateNoWindow = true
            }
        };

        process.OutputDataReceived += (_, e) =>
        {
            if (!string.IsNullOrWhiteSpace(e.Data))
            {
                Log.Debug("{Output}", e.Data);
            }
        };
        process.ErrorDataReceived += (_, e) =>
        {
            if (!string.IsNullOrWhiteSpace(e.Data))
            {
                Log.Debug("{Output}", e.Data);
            }
        };

        process.Start();
        process.BeginOutputReadLine();
        process.BeginErrorReadLine();
        await process.WaitForExitAsync();

        if (process.ExitCode == 0)
        {
            return true;
        }

        Log.Error("{File} exited with code {Code}", fileName, process.ExitCode);
        return false;
    }
}
