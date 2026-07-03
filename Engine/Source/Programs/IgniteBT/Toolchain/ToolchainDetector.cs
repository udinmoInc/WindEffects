using System.Runtime.InteropServices;
using Serilog;

namespace IgniteBT.Toolchain;

/// <summary>
/// Detects and configures build toolchains (compilers, linkers) on the system.
/// </summary>
public class ToolchainDetector
{
    /// <summary>
    /// Detects the available compiler for the current platform.
    /// </summary>
    public static DetectedCompiler DetectCompiler()
    {
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            return DetectMSVC();
        }
        else if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
        {
            return DetectGCCOrClangLinux();
        }
        else if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
        {
            return DetectClangMac();
        }

        Log.Warning("Unknown platform, defaulting to no compiler");
        return new DetectedCompiler { Type = CompilerType.None, Path = string.Empty };
    }

    /// <summary>
    /// Detects MSVC compiler on Windows.
    /// </summary>
    private static DetectedCompiler DetectMSVC()
    {
        Log.Information("Detecting MSVC compiler on Windows");

        // Try to find Visual Studio installations using vswhere
        var vswherePath = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),
            "Microsoft Visual Studio",
            "Installer",
            "vswhere.exe"
        );

        if (!File.Exists(vswherePath))
        {
            Log.Warning("vswhere.exe not found at {Path}", vswherePath);
            return DetectMSVCFallback();
        }

        try
        {
            var psi = new System.Diagnostics.ProcessStartInfo
            {
                FileName = vswherePath,
                Arguments = "-latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath",
                RedirectStandardOutput = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var process = System.Diagnostics.Process.Start(psi);
            if (process == null)
            {
                return DetectMSVCFallback();
            }

            var output = process.StandardOutput.ReadToEnd();
            process.WaitForExit();

            if (process.ExitCode != 0 || string.IsNullOrWhiteSpace(output))
            {
                Log.Warning("vswhere failed to find Visual Studio installation");
                return DetectMSVCFallback();
            }

            var vsPath = output.Trim();
            Log.Information("Found Visual Studio at {Path}", vsPath);

            // Find vcvarsall.bat
            var vcvarsallPath = Path.Combine(vsPath, "VC", "Auxiliary", "Build", "vcvarsall.bat");
            if (!File.Exists(vcvarsallPath))
            {
                Log.Warning("vcvarsall.bat not found at {Path}", vcvarsallPath);
                return DetectMSVCFallback();
            }

            // Find cl.exe
            var vcToolsPath = Path.Combine(vsPath, "VC", "Tools", "MSVC");
            if (!Directory.Exists(vcToolsPath))
            {
                Log.Warning("VC Tools directory not found at {Path}", vcToolsPath);
                return DetectMSVCFallback();
            }

            // Get the latest MSVC version
            var latestVersion = Directory.GetDirectories(vcToolsPath)
                .OrderByDescending(d => d)
                .FirstOrDefault();

            if (latestVersion == null)
            {
                Log.Warning("No MSVC version found");
                return DetectMSVCFallback();
            }

            Log.Information("Using MSVC version: {Version}", Path.GetFileName(latestVersion));

            // Find the host architecture
            var hostArch = RuntimeInformation.OSArchitecture == Architecture.Arm64 ? "arm64" : "x64";
            var binPath = Path.Combine(latestVersion, "bin", $"Host{hostArch}", hostArch);

            if (!Directory.Exists(binPath))
            {
                Log.Warning("Bin directory not found at {Path}", binPath);
                return DetectMSVCFallback();
            }

            var clPath = Path.Combine(binPath, "cl.exe");
            if (!File.Exists(clPath))
            {
                Log.Warning("cl.exe not found at {Path}", clPath);
                return DetectMSVCFallback();
            }

            Log.Information("Found cl.exe at {Path}", clPath);

            return new DetectedCompiler
            {
                Type = CompilerType.MSVC,
                Path = clPath,
                Version = ExtractMSVCVersion(latestVersion),
                IncludePath = Path.Combine(latestVersion, "include"),
                LibraryPath = Path.Combine(latestVersion, "lib", hostArch),
                VcVarsAllPath = vcvarsallPath,
                VsInstallPath = vsPath
            };
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Error detecting MSVC compiler");
            return DetectMSVCFallback();
        }
    }

    /// <summary>
    /// Fallback MSVC detection using PATH environment variable.
    /// </summary>
    private static DetectedCompiler DetectMSVCFallback()
    {
        Log.Information("Attempting fallback MSVC detection via PATH");

        var pathEnv = Environment.GetEnvironmentVariable("PATH") ?? string.Empty;
        var paths = pathEnv.Split(Path.PathSeparator);

        foreach (var path in paths)
        {
            var clPath = Path.Combine(path, "cl.exe");
            if (File.Exists(clPath))
            {
                Log.Information("Found cl.exe in PATH at {Path}", clPath);
                return new DetectedCompiler
                {
                    Type = CompilerType.MSVC,
                    Path = clPath,
                    Version = "Unknown"
                };
            }
        }

        Log.Warning("MSVC compiler not found in PATH");
        return new DetectedCompiler { Type = CompilerType.None, Path = string.Empty };
    }

    /// <summary>
    /// Detects GCC or Clang on Linux.
    /// </summary>
    private static DetectedCompiler DetectGCCOrClangLinux()
    {
        Log.Information("Detecting compiler on Linux");

        // Try clang first
        if (TryFindCompiler("clang", out var clangPath) && clangPath != null)
        {
            Log.Information("Found clang at {Path}", clangPath);
            return new DetectedCompiler
            {
                Type = CompilerType.Clang,
                Path = clangPath,
                Version = GetCompilerVersion(clangPath)
            };
        }

        // Try gcc
        if (TryFindCompiler("gcc", out var gccPath) && gccPath != null)
        {
            Log.Information("Found gcc at {Path}", gccPath);
            return new DetectedCompiler
            {
                Type = CompilerType.GCC,
                Path = gccPath,
                Version = GetCompilerVersion(gccPath)
            };
        }

        Log.Warning("No compiler found on Linux");
        return new DetectedCompiler { Type = CompilerType.None, Path = string.Empty };
    }

    /// <summary>
    /// Detects Clang on macOS.
    /// </summary>
    private static DetectedCompiler DetectClangMac()
    {
        Log.Information("Detecting compiler on macOS");

        if (TryFindCompiler("clang", out var clangPath) && clangPath != null)
        {
            Log.Information("Found clang at {Path}", clangPath);
            return new DetectedCompiler
            {
                Type = CompilerType.Clang,
                Path = clangPath,
                Version = GetCompilerVersion(clangPath)
            };
        }

        Log.Warning("Clang not found on macOS");
        return new DetectedCompiler { Type = CompilerType.None, Path = string.Empty };
    }

    /// <summary>
    /// Tries to find a compiler executable in the system PATH.
    /// </summary>
    private static bool TryFindCompiler(string compilerName, out string? path)
    {
        path = null;

        var pathEnv = Environment.GetEnvironmentVariable("PATH") ?? string.Empty;
        var paths = pathEnv.Split(Path.PathSeparator);

        foreach (var dir in paths)
        {
            var compilerPath = Path.Combine(dir, compilerName);
            if (File.Exists(compilerPath))
            {
                path = compilerPath;
                return true;
            }
        }

        return false;
    }

    /// <summary>
    /// Extracts the MSVC version from the version directory path.
    /// </summary>
    private static string ExtractMSVCVersion(string versionPath)
    {
        var version = Path.GetFileName(versionPath);
        // MSVC version format: 14.xx.xxxxx
        return version;
    }

    /// <summary>
    /// Gets the compiler version by running it with --version flag.
    /// </summary>
    private static string GetCompilerVersion(string compilerPath)
    {
        try
        {
            var psi = new System.Diagnostics.ProcessStartInfo
            {
                FileName = compilerPath,
                Arguments = "--version",
                RedirectStandardOutput = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var process = System.Diagnostics.Process.Start(psi);
            if (process == null)
            {
                return "Unknown";
            }

            var output = process.StandardOutput.ReadToEnd();
            process.WaitForExit();

            // Extract version from first line
            var firstLine = output.Split('\n').FirstOrDefault();
            return firstLine ?? "Unknown";
        }
        catch
        {
            return "Unknown";
        }
    }

    /// <summary>
    /// Sets up the MSVC environment by running vcvarsall.bat and returning the environment variables.
    /// </summary>
    public static Dictionary<string, string> SetupMSVCEnvironment(string vcvarsallPath, string architecture = "x64")
    {
        var envVars = new Dictionary<string, string>();

        try
        {
            Log.Information("Setting up MSVC environment using vcvarsall.bat");

            // Create a temporary batch file that runs vcvarsall.bat and outputs environment variables
            var tempBatPath = Path.Combine(Path.GetTempPath(), $"ignitebt_vcvars_{Guid.NewGuid()}.bat");
            var tempEnvPath = Path.Combine(Path.GetTempPath(), $"ignitebt_env_{Guid.NewGuid()}.txt");

            var batchContent = $@"@echo off
call ""{vcvarsallPath}"" {architecture}
set > ""{tempEnvPath}""
";
            File.WriteAllText(tempBatPath, batchContent);

            var psi = new System.Diagnostics.ProcessStartInfo
            {
                FileName = tempBatPath,
                UseShellExecute = false,
                CreateNoWindow = true,
                WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden
            };

            using var process = System.Diagnostics.Process.Start(psi);
            if (process == null)
            {
                Log.Warning("Failed to run vcvarsall.bat");
                return envVars;
            }

            process.WaitForExit();

            // Read the environment variables
            if (File.Exists(tempEnvPath))
            {
                var envLines = File.ReadAllLines(tempEnvPath);
                foreach (var line in envLines)
                {
                    var parts = line.Split('=', 2);
                    if (parts.Length == 2)
                    {
                        envVars[parts[0]] = parts[1];
                    }
                }

                File.Delete(tempEnvPath);
            }

            // Clean up temp batch file
            if (File.Exists(tempBatPath))
            {
                File.Delete(tempBatPath);
            }

            Log.Information("MSVC environment setup complete with {Count} variables", envVars.Count);
            return envVars;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Failed to set up MSVC environment");
            return envVars;
        }
    }
}

/// <summary>
/// Represents a detected compiler.
/// </summary>
public class DetectedCompiler
{
    /// <summary>
    /// Compiler type.
    /// </summary>
    public CompilerType Type { get; set; }

    /// <summary>
    /// Path to the compiler executable.
    /// </summary>
    public string Path { get; set; } = string.Empty;

    /// <summary>
    /// Compiler version string.
    /// </summary>
    public string Version { get; set; } = string.Empty;

    /// <summary>
    /// Include path (for MSVC).
    /// </summary>
    public string? IncludePath { get; set; }

    /// <summary>
    /// Library path (for MSVC).
    /// </summary>
    public string? LibraryPath { get; set; }

    /// <summary>
    /// Path to vcvarsall.bat (for MSVC environment setup).
    /// </summary>
    public string? VcVarsAllPath { get; set; }

    /// <summary>
    /// Visual Studio installation path (for MSVC).
    /// </summary>
    public string? VsInstallPath { get; set; }
}

/// <summary>
/// Compiler type enumeration.
/// </summary>
public enum CompilerType
{
    None,
    MSVC,
    GCC,
    Clang
}
