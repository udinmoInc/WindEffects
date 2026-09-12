using System.Runtime.InteropServices;
using Serilog;

namespace IgniteBT.Build.Toolchain;

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
    /// Finds vswhere.exe dynamically across installer directories, PATH, environment, and fixed drives.
    /// </summary>
    public static string? FindVsWhere()
    {
        var candidates = new List<string>();

        var pfX86 = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86);
        if (!string.IsNullOrEmpty(pfX86))
            candidates.Add(Path.Combine(pfX86, "Microsoft Visual Studio", "Installer", "vswhere.exe"));

        var pf = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);
        if (!string.IsNullOrEmpty(pf))
            candidates.Add(Path.Combine(pf, "Microsoft Visual Studio", "Installer", "vswhere.exe"));

        var vsDir = Environment.GetEnvironmentVariable("VSINSTALLDIR");
        if (!string.IsNullOrEmpty(vsDir))
            candidates.Add(Path.Combine(vsDir, "Installer", "vswhere.exe"));

        // PATH lookup
        if (TryFindCompiler("vswhere.exe", out var pathInPath) && !string.IsNullOrEmpty(pathInPath))
            candidates.Add(pathInPath);
        if (TryFindCompiler("vswhere", out var pathInPathNoExt) && !string.IsNullOrEmpty(pathInPathNoExt))
            candidates.Add(pathInPathNoExt);

        foreach (var c in candidates)
        {
            if (File.Exists(c)) return c;
        }

        // Drive roots scan for Installer/vswhere.exe
        try
        {
            foreach (var drive in DriveInfo.GetDrives().Where(d => d.DriveType == DriveType.Fixed && d.IsReady))
            {
                var root = drive.RootDirectory.FullName;
                var commonSubdirs = new[]
                {
                    Path.Combine(root, "Program Files (x86)", "Microsoft Visual Studio", "Installer", "vswhere.exe"),
                    Path.Combine(root, "Program Files", "Microsoft Visual Studio", "Installer", "vswhere.exe"),
                    Path.Combine(root, "Microsoft Visual Studio", "Installer", "vswhere.exe"),
                    Path.Combine(root, "vs", "Installer", "vswhere.exe"),
                    Path.Combine(root, "vs", "vswhere.exe")
                };
                foreach (var s in commonSubdirs)
                {
                    if (File.Exists(s)) return s;
                }
            }
        }
        catch
        {
            // Ignore filesystem permissions exceptions on drive roots
        }

        return null;
    }

    /// <summary>
    /// Constructs a DetectedCompiler from a Visual Studio installation directory.
    /// </summary>
    public static DetectedCompiler? ResolveMSVCFromVsInstallPath(string vsPath)
    {
        if (string.IsNullOrEmpty(vsPath) || !Directory.Exists(vsPath))
            return null;

        var vcvarsallPath = Path.Combine(vsPath, "VC", "Auxiliary", "Build", "vcvarsall.bat");
        var vcToolsPath = Path.Combine(vsPath, "VC", "Tools", "MSVC");

        if (!File.Exists(vcvarsallPath) || !Directory.Exists(vcToolsPath))
            return null;

        var latestVersion = Directory.GetDirectories(vcToolsPath)
            .OrderByDescending(d => d, StringComparer.OrdinalIgnoreCase)
            .FirstOrDefault();

        if (latestVersion == null) return null;

        var hostArch = RuntimeInformation.OSArchitecture == Architecture.Arm64 ? "arm64" : "x64";
        var binPath = Path.Combine(latestVersion, "bin", $"Host{hostArch}", hostArch);
        var clPath = Path.Combine(binPath, "cl.exe");

        if (!File.Exists(clPath))
        {
            binPath = Path.Combine(latestVersion, "bin", "Hostx64", "x64");
            clPath = Path.Combine(binPath, "cl.exe");
            if (!File.Exists(clPath)) return null;
        }

        Log.Information("Resolved MSVC at {Path} (Version: {Version})", clPath, Path.GetFileName(latestVersion));

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

    /// <summary>
    /// Detects MSVC compiler on Windows.
    /// </summary>
    private static DetectedCompiler DetectMSVC()
    {
        Log.Information("Detecting MSVC compiler on Windows");

        var vswherePath = FindVsWhere();
        if (!string.IsNullOrEmpty(vswherePath) && File.Exists(vswherePath))
        {
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
                if (process != null)
                {
                    var output = process.StandardOutput.ReadToEnd();
                    process.WaitForExit();

                    if (process.ExitCode == 0 && !string.IsNullOrWhiteSpace(output))
                    {
                        var vsPath = output.Trim();
                        var detected = ResolveMSVCFromVsInstallPath(vsPath);
                        if (detected != null)
                            return detected;
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "vswhere execution encountered error");
            }
        }

        return DetectMSVCFallback();
    }

    /// <summary>
    /// Fallback MSVC detection using VSINSTALLDIR, PATH, cl.exe parent hierarchy, and drive roots.
    /// </summary>
    private static DetectedCompiler DetectMSVCFallback()
    {
        Log.Information("Attempting fallback MSVC detection via VSINSTALLDIR, PATH, and drive scanning");

        // 1. Try VSINSTALLDIR environment variable
        var vsEnvDir = Environment.GetEnvironmentVariable("VSINSTALLDIR");
        if (!string.IsNullOrEmpty(vsEnvDir))
        {
            var detectedEnv = ResolveMSVCFromVsInstallPath(vsEnvDir);
            if (detectedEnv != null) return detectedEnv;
        }

        // 2. Search PATH for cl.exe and infer VC installation tree
        var pathEnv = Environment.GetEnvironmentVariable("PATH") ?? string.Empty;
        var paths = pathEnv.Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);

        foreach (var p in paths)
        {
            var clPath = Path.Combine(p, "cl.exe");
            if (File.Exists(clPath))
            {
                Log.Information("Found cl.exe in PATH at {Path}", clPath);
                var resolvedFromCl = InferMSVCFromClPath(clPath);
                if (resolvedFromCl != null) return resolvedFromCl;
            }
        }

        // 3. Scan all fixed drives for Visual Studio / VC roots
        try
        {
            foreach (var drive in DriveInfo.GetDrives().Where(d => d.DriveType == DriveType.Fixed && d.IsReady))
            {
                var root = drive.RootDirectory.FullName;
                var commonVcRoots = new[]
                {
                    Path.Combine(root, "vs"),
                    Path.Combine(root, "Microsoft Visual Studio"),
                    Path.Combine(root, "Program Files", "Microsoft Visual Studio", "2022", "Community"),
                    Path.Combine(root, "Program Files", "Microsoft Visual Studio", "2022", "Professional"),
                    Path.Combine(root, "Program Files", "Microsoft Visual Studio", "2022", "Enterprise"),
                    Path.Combine(root, "Program Files (x86)", "Microsoft Visual Studio", "2019", "Community"),
                    Path.Combine(root, "Program Files (x86)", "Microsoft Visual Studio", "2019", "Professional"),
                    Path.Combine(root, "Program Files (x86)", "Microsoft Visual Studio", "2019", "Enterprise"),
                    Path.Combine(root, "Program Files (x86)", "Microsoft Visual Studio", "2022", "BuildTools"),
                    Path.Combine(root, "Program Files (x86)", "Microsoft Visual Studio", "2019", "BuildTools")
                };

                foreach (var vcRoot in commonVcRoots)
                {
                    var detectedRoot = ResolveMSVCFromVsInstallPath(vcRoot);
                    if (detectedRoot != null) return detectedRoot;
                }
            }
        }
        catch (Exception ex)
        {
            Log.Debug(ex, "Exception while scanning fixed drive roots for MSVC");
        }

        Log.Warning("MSVC compiler not found");
        return new DetectedCompiler { Type = CompilerType.None, Path = string.Empty };
    }

    /// <summary>
    /// Infers MSVC version, vcvarsall.bat, include/lib paths by navigating up from cl.exe directory.
    /// </summary>
    public static DetectedCompiler? InferMSVCFromClPath(string clPath)
    {
        try
        {
            var fullCl = Path.GetFullPath(clPath);
            var dir = Path.GetDirectoryName(fullCl);
            if (string.IsNullOrEmpty(dir)) return null;

            // Walk up to find MSVC version dir (parent of bin/Host.../...)
            var current = new DirectoryInfo(dir);
            DirectoryInfo? msvcVersionDir = null;
            DirectoryInfo? vcDir = null;
            DirectoryInfo? vsInstallDir = null;

            while (current != null)
            {
                if (current.Parent?.Name.Equals("MSVC", StringComparison.OrdinalIgnoreCase) == true &&
                    current.Parent.Parent?.Name.Equals("Tools", StringComparison.OrdinalIgnoreCase) == true)
                {
                    msvcVersionDir = current;
                }

                if (current.Name.Equals("VC", StringComparison.OrdinalIgnoreCase) ||
                    File.Exists(Path.Combine(current.FullName, "Auxiliary", "Build", "vcvarsall.bat")))
                {
                    vcDir = current;
                    vsInstallDir = current.Parent;
                    break;
                }

                current = current.Parent;
            }

            var hostArch = RuntimeInformation.OSArchitecture == Architecture.Arm64 ? "arm64" : "x64";
            var vcvarsallPath = vcDir != null
                ? Path.Combine(vcDir.FullName, "Auxiliary", "Build", "vcvarsall.bat")
                : null;

            if (vcvarsallPath != null && !File.Exists(vcvarsallPath))
                vcvarsallPath = null;

            var versionStr = msvcVersionDir?.Name ?? "Unknown";
            var includePath = msvcVersionDir != null ? Path.Combine(msvcVersionDir.FullName, "include") : null;
            var libPath = msvcVersionDir != null ? Path.Combine(msvcVersionDir.FullName, "lib", hostArch) : null;

            return new DetectedCompiler
            {
                Type = CompilerType.MSVC,
                Path = fullCl,
                Version = versionStr,
                IncludePath = includePath,
                LibraryPath = libPath,
                VcVarsAllPath = vcvarsallPath,
                VsInstallPath = vsInstallDir?.FullName
            };
        }
        catch
        {
            return null;
        }
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
