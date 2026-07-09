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
    private const string FreeTypeRemote = "https://github.com/freetype/freetype.git";
    private const string FreeTypeTag = "VER-2-13-2";
    private const string MsdfAtlasGenRemote = "https://github.com/Chlumsky/msdf-atlas-gen.git";
    private const string LunaSvgRemote = "https://github.com/sammycage/lunasvg.git";
    private const string LunaSvgTag = "v2.3.9";
    private const string MsvcDynamicRuntimeMarker = ".msvc_md_runtime";
    private const string MsvcDynamicRuntimeCmake = "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL";

    public static async Task<bool> EnsureRequiredAsync(string engineRoot, int jobs = 0)
    {
        var thirdPartyRoot = Path.Combine(engineRoot, "ThirdParty");
        Directory.CreateDirectory(thirdPartyRoot);

        var glmOk = await EnsureGlmAsync(Path.Combine(thirdPartyRoot, "glm"));
        var sdlOk = await EnsureSdl3Async(Path.Combine(thirdPartyRoot, "SDL3"), jobs);
        var freetypeOk = await EnsureFreeTypeAsync(Path.Combine(thirdPartyRoot, "freetype"), jobs);
        var msdfgenOk = await EnsureMsdfAtlasGenAsync(Path.Combine(thirdPartyRoot, "msdf-atlas-gen"), jobs);
        var lunaSvgOk = await EnsureLunaSvgAsync(Path.Combine(thirdPartyRoot, "lunasvg"), jobs);

        if (!glmOk || !sdlOk || !freetypeOk || !msdfgenOk || !lunaSvgOk)
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

    private static async Task<bool> EnsureFreeTypeAsync(string freetypePath, int jobs)
    {
        var headerPath = Path.Combine(freetypePath, "include", "ft2build.h");
        var releaseLib = Path.Combine(freetypePath, "lib", "freetype.lib");
        var runtimeMarker = Path.Combine(freetypePath, "lib", MsvcDynamicRuntimeMarker);
        if (File.Exists(headerPath) && File.Exists(releaseLib) && File.Exists(runtimeMarker))
        {
            return true;
        }

        if (!File.Exists(headerPath))
        {
            Log.Warning("FreeType missing at {Path}. Cloning {Tag}...", freetypePath, FreeTypeTag);
            if (Directory.Exists(freetypePath))
            {
                Directory.Delete(freetypePath, recursive: true);
            }

            if (!await RunProcessAsync("git", $"clone --depth 1 --branch {FreeTypeTag} {FreeTypeRemote} \"{freetypePath}\""))
            {
                return false;
            }
        }

        var buildDir = Path.Combine(freetypePath, "build");
        var configureArgs =
            $"-S \"{freetypePath}\" -B \"{buildDir}\" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF " +
            "-DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_PNG=ON -DFT_DISABLE_ZLIB=ON -DFT_DISABLE_BZIP2=ON -DFT_DISABLE_BROTLI=ON " +
            $"\"-DCMAKE_POLICY_VERSION_MINIMUM=3.5\" {MsvcDynamicRuntimeCmake}";
        if (!await RunProcessAsync("cmake", configureArgs))
        {
            return false;
        }

        var parallel = jobs > 0 ? jobs : Math.Max(1, Environment.ProcessorCount);
        if (!await RunProcessAsync("cmake", $"--build \"{buildDir}\" --config Release -j {parallel}"))
        {
            return false;
        }

        Directory.CreateDirectory(Path.Combine(freetypePath, "lib"));
        var builtLib = Path.Combine(buildDir, "Release", "freetype.lib");
        if (!File.Exists(builtLib))
        {
            builtLib = Path.Combine(buildDir, "freetype.lib");
        }
        if (!File.Exists(builtLib))
        {
            Log.Error("FreeType build finished but freetype.lib was not produced.");
            return false;
        }

        File.Copy(builtLib, releaseLib, overwrite: true);
        File.WriteAllText(runtimeMarker, "1");
        Log.Information("FreeType bootstrapped at {Path}", releaseLib);
        return true;
    }

    private static async Task<bool> EnsureMsdfAtlasGenAsync(string msdfAtlasPath, int jobs)
    {
        var headerPath = Path.Combine(msdfAtlasPath, "msdf-atlas-gen", "msdf-atlas-gen.h");
        var releaseLib = Path.Combine(msdfAtlasPath, "lib", "msdf-atlas-gen.lib");
        var configHeaderPath = Path.Combine(msdfAtlasPath, "msdfgen", "msdfgen-config.h");
        var runtimeMarker = Path.Combine(msdfAtlasPath, "lib", MsvcDynamicRuntimeMarker);
        if (File.Exists(headerPath) && File.Exists(releaseLib) && File.Exists(configHeaderPath) && File.Exists(runtimeMarker))
        {
            return true;
        }

        if (!File.Exists(headerPath))
        {
            Log.Warning("msdf-atlas-gen missing at {Path}. Cloning...", msdfAtlasPath);
            if (Directory.Exists(msdfAtlasPath))
            {
                Directory.Delete(msdfAtlasPath, recursive: true);
            }

            if (!await RunProcessAsync("git", $"clone --depth 1 --recursive {MsdfAtlasGenRemote} \"{msdfAtlasPath}\""))
            {
                return false;
            }
        }

        var freetypeRoot = Path.GetFullPath(Path.Combine(msdfAtlasPath, "..", "freetype"));
        var freetypeLib = Path.Combine(freetypeRoot, "lib", "freetype.lib");
        if (!File.Exists(freetypeLib))
        {
            Log.Error("msdf-atlas-gen requires FreeType to be built first ({Path}).", freetypeLib);
            return false;
        }

        var buildDir = Path.Combine(msdfAtlasPath, "build");
        var configureArgs =
            $"-S \"{msdfAtlasPath}\" -B \"{buildDir}\" -DCMAKE_BUILD_TYPE=Release " +
            "-DMSDF_ATLAS_BUILD_STANDALONE=OFF -DMSDF_ATLAS_USE_VCPKG=OFF -DMSDF_ATLAS_USE_SKIA=OFF " +
            "-DMSDF_ATLAS_DYNAMIC_RUNTIME=ON -DBUILD_SHARED_LIBS=OFF -DMSDFGEN_DISABLE_PNG=ON " +
            $"\"-DCMAKE_POLICY_VERSION_MINIMUM=3.5\" {MsvcDynamicRuntimeCmake} " +
            $"-DFREETYPE_INCLUDE_DIRS=\"{Path.Combine(freetypeRoot, "include").Replace('\\', '/')}\" " +
            $"-DFREETYPE_LIBRARY=\"{freetypeLib.Replace('\\', '/')}\"";
        if (!await RunProcessAsync("cmake", configureArgs))
        {
            return false;
        }

        var parallel = jobs > 0 ? jobs : Math.Max(1, Environment.ProcessorCount);
        if (!await RunProcessAsync("cmake", $"--build \"{buildDir}\" --config Release -j {parallel}"))
        {
            return false;
        }

        Directory.CreateDirectory(Path.Combine(msdfAtlasPath, "lib"));
        var libCopies = new (string Source, string Name)[]
        {
            (Path.Combine(buildDir, "Release", "msdf-atlas-gen.lib"), "msdf-atlas-gen.lib"),
            (Path.Combine(buildDir, "msdfgen", "Release", "msdfgen-core.lib"), "msdfgen-core.lib"),
            (Path.Combine(buildDir, "msdfgen", "Release", "msdfgen-ext.lib"), "msdfgen-ext.lib"),
        };
        foreach (var (source, name) in libCopies)
        {
            if (!File.Exists(source))
            {
                Log.Error("msdf-atlas-gen build finished but {Lib} was not produced.", source);
                return false;
            }
            File.Copy(source, Path.Combine(msdfAtlasPath, "lib", name), overwrite: true);
        }

        EnsureMsdfgenConfigHeader(msdfAtlasPath);
        File.WriteAllText(runtimeMarker, "1");

        Log.Information("msdf-atlas-gen bootstrapped at {Path}", msdfAtlasPath);
        return true;
    }

    private static void EnsureMsdfgenConfigHeader(string msdfAtlasPath)
    {
        var configHeaderPath = Path.Combine(msdfAtlasPath, "msdfgen", "msdfgen-config.h");
        if (File.Exists(configHeaderPath))
        {
            return;
        }

        Directory.CreateDirectory(Path.GetDirectoryName(configHeaderPath)!);
        File.WriteAllText(configHeaderPath,
            """
            #pragma once

            #define MSDFGEN_PUBLIC
            #define MSDFGEN_EXT_PUBLIC

            #define MSDFGEN_VERSION 1.13.0
            #define MSDFGEN_VERSION_MAJOR 1
            #define MSDFGEN_VERSION_MINOR 13
            #define MSDFGEN_VERSION_REVISION 0
            #define MSDFGEN_COPYRIGHT_YEAR 2026

            #define MSDFGEN_USE_CPP11
            #define MSDFGEN_EXTENSIONS
            #define MSDFGEN_DISABLE_SVG
            #define MSDFGEN_DISABLE_PNG
            """);
    }

    private static async Task<bool> EnsureLunaSvgAsync(string lunaSvgPath, int jobs)
    {
        var headerPath = Path.Combine(lunaSvgPath, "include", "lunasvg.h");
        var releaseLib = Path.Combine(lunaSvgPath, "lib", "lunasvg.lib");
        var runtimeMarker = Path.Combine(lunaSvgPath, "lib", MsvcDynamicRuntimeMarker);
        if (File.Exists(headerPath) && File.Exists(releaseLib) && File.Exists(runtimeMarker))
        {
            return true;
        }

        if (!File.Exists(headerPath))
        {
            Log.Warning("LunaSVG missing at {Path}. Cloning {Tag}...", lunaSvgPath, LunaSvgTag);
            if (Directory.Exists(lunaSvgPath))
            {
                Directory.Delete(lunaSvgPath, recursive: true);
            }

            if (!await RunProcessAsync("git", $"clone --depth 1 --branch {LunaSvgTag} {LunaSvgRemote} \"{lunaSvgPath}\""))
            {
                return false;
            }
        }

        var buildDir = Path.Combine(lunaSvgPath, "build");
        var configureArgs =
            $"-S \"{lunaSvgPath}\" -B \"{buildDir}\" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF " +
            $"\"-DCMAKE_POLICY_VERSION_MINIMUM=3.5\" {MsvcDynamicRuntimeCmake}";
        if (!await RunProcessAsync("cmake", configureArgs))
        {
            return false;
        }

        var parallel = jobs > 0 ? jobs : Math.Max(1, Environment.ProcessorCount);
        if (!await RunProcessAsync("cmake", $"--build \"{buildDir}\" --config Release -j {parallel}"))
        {
            return false;
        }

        Directory.CreateDirectory(Path.Combine(lunaSvgPath, "lib"));
        foreach (var libName in new[] { "lunasvg.lib" })
        {
            var builtLib = Path.Combine(buildDir, "Release", libName);
            if (!File.Exists(builtLib))
            {
                builtLib = Path.Combine(buildDir, libName);
            }
            if (!File.Exists(builtLib))
            {
                Log.Error("LunaSVG build finished but {Lib} was not produced.", libName);
                return false;
            }
            File.Copy(builtLib, Path.Combine(lunaSvgPath, "lib", libName), overwrite: true);
        }

        File.WriteAllText(runtimeMarker, "1");
        Log.Information("LunaSVG bootstrapped at {Path}", lunaSvgPath);
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
