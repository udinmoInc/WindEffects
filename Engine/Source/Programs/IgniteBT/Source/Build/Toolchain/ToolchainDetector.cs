using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text.Json;
using Serilog;

namespace IgniteBT.Build.Toolchain;

/// <summary>
/// Detects, validates, and caches build toolchains (compilers, linkers) on the system.
/// Implements thread-safe in-process caching and persistent disk caching to eliminate redundant discovery scans.
/// </summary>
public class ToolchainDetector
{
    private static readonly object _lock = new();
    private static DetectedCompiler? _cachedCompiler;
    private static bool _loggedDiscoveryAttempt = false;

    private const string CacheFileName = "IgniteBT.ToolchainCache.json";

    /// <summary>
    /// Detects or retrieves the cached compiler for the current platform.
    /// Thread-safe, process-cached, and backed by persistent storage.
    /// </summary>
    public static DetectedCompiler DetectCompiler(bool forceRescan = false)
    {
        lock (_lock)
        {
            // 1. In-process cache check
            if (!forceRescan && _cachedCompiler != null && ValidateMSVC(_cachedCompiler))
            {
                Log.Debug("MSVC toolchain reused from in-process cache ({Path})", _cachedCompiler.Path);
                return _cachedCompiler;
            }

            // 2. Persistent disk cache check
            if (!forceRescan)
            {
                var persistentCompiler = LoadPersistentCache();
                if (persistentCompiler != null && ValidateMSVC(persistentCompiler))
                {
                    persistentCompiler.IsValid = true;
                    _cachedCompiler = persistentCompiler;
                    if (!_loggedDiscoveryAttempt)
                    {
                        Log.Information("MSVC toolchain loaded from persistent cache: {Path} (v{Version})", persistentCompiler.Path, persistentCompiler.Version);
                        _loggedDiscoveryAttempt = true;
                    }
                    else
                    {
                        Log.Debug("MSVC toolchain reused from persistent cache ({Path})", persistentCompiler.Path);
                    }
                    return _cachedCompiler;
                }
            }

            // 3. Full discovery cycle
            var totalSw = Stopwatch.StartNew();

            if (!_loggedDiscoveryAttempt)
            {
                Log.Information("Detecting compiler for current platform");
                _loggedDiscoveryAttempt = true;
            }

            DetectedCompiler compiler;
            var discoverySw = Stopwatch.StartNew();
            if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            {
                compiler = DiscoverMSVC();
            }
            else if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            {
                compiler = DetectGCCOrClangLinux();
            }
            else if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
            {
                compiler = DetectClangMac();
            }
            else
            {
                Log.Warning("Unknown platform, defaulting to no compiler");
                compiler = new DetectedCompiler { Type = CompilerType.None, Path = string.Empty };
            }
            discoverySw.Stop();

            var validationSw = Stopwatch.StartNew();
            var isValid = ValidateMSVC(compiler);
            validationSw.Stop();

            totalSw.Stop();

            if (isValid)
            {
                compiler.IsValid = true;
                compiler.LastValidatedUtc = DateTime.UtcNow;
                _cachedCompiler = compiler;
                SavePersistentCache(compiler);

                Log.Information("MSVC toolchain discovered in {DiscoveryMs}ms, validated in {ValidationMs}ms: {Path} (v{Version})",
                    discoverySw.ElapsedMilliseconds,
                    validationSw.ElapsedMilliseconds,
                    compiler.Path,
                    compiler.Version);
                Log.Information("MSVC toolchain cached");
            }
            else
            {
                Log.Warning("Compiler detection failed or produced invalid compiler");
                _cachedCompiler = compiler;
            }

            return _cachedCompiler;
        }
    }

    /// <summary>
    /// Validates whether a detected compiler executable exists and is functional.
    /// </summary>
    public static bool ValidateMSVC(DetectedCompiler? compiler)
    {
        if (compiler == null) return false;
        if (compiler.Type == CompilerType.None) return false;
        if (string.IsNullOrWhiteSpace(compiler.Path)) return false;
        if (!File.Exists(compiler.Path)) return false;
        return true;
    }

    /// <summary>
    /// Detects MSVC compiler on Windows using a deterministic priority chain.
    /// </summary>
    private static DetectedCompiler DiscoverMSVC()
    {
        Log.Information("Detecting MSVC compiler on Windows");

        // Priority 1: Environment variable VSINSTALLDIR
        var vsEnvDir = Environment.GetEnvironmentVariable("VSINSTALLDIR");
        if (!string.IsNullOrEmpty(vsEnvDir) && Directory.Exists(vsEnvDir))
        {
            var detectedEnv = ResolveMSVCFromVsInstallPath(vsEnvDir);
            if (ValidateMSVC(detectedEnv))
            {
                Log.Information("Found MSVC via VSINSTALLDIR at {Path}", detectedEnv!.Path);
                return detectedEnv;
            }
        }

        // Priority 2: PATH lookup (if cl.exe is in PATH, immediately accept & infer; DO NOT scan drives)
        var pathEnv = Environment.GetEnvironmentVariable("PATH") ?? string.Empty;
        var paths = pathEnv.Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
        foreach (var p in paths)
        {
            var clPath = Path.Combine(p, "cl.exe");
            if (File.Exists(clPath))
            {
                Log.Information("Found cl.exe in PATH at {Path}", clPath);
                var resolvedFromCl = InferMSVCFromClPath(clPath);
                if (ValidateMSVC(resolvedFromCl))
                {
                    return resolvedFromCl!;
                }
            }
        }

        // Priority 3: Visual Studio Installer (vswhere) in standard paths
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
                        if (ValidateMSVC(detected))
                        {
                            return detected!;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "vswhere execution encountered error");
            }
        }

        // Priority 4: Well-known Visual Studio installation directories (no drive root scan)
        var pf = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);
        var pfX86 = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86);
        var standardVsDirs = new List<string>();

        if (!string.IsNullOrEmpty(pf))
        {
            standardVsDirs.Add(Path.Combine(pf, "Microsoft Visual Studio", "2022", "Community"));
            standardVsDirs.Add(Path.Combine(pf, "Microsoft Visual Studio", "2022", "Professional"));
            standardVsDirs.Add(Path.Combine(pf, "Microsoft Visual Studio", "2022", "Enterprise"));
            standardVsDirs.Add(Path.Combine(pf, "Microsoft Visual Studio", "2022", "BuildTools"));
        }
        if (!string.IsNullOrEmpty(pfX86))
        {
            standardVsDirs.Add(Path.Combine(pfX86, "Microsoft Visual Studio", "2019", "Community"));
            standardVsDirs.Add(Path.Combine(pfX86, "Microsoft Visual Studio", "2019", "Professional"));
            standardVsDirs.Add(Path.Combine(pfX86, "Microsoft Visual Studio", "2019", "Enterprise"));
            standardVsDirs.Add(Path.Combine(pfX86, "Microsoft Visual Studio", "2019", "BuildTools"));
        }

        foreach (var vsDirCandidate in standardVsDirs)
        {
            if (Directory.Exists(vsDirCandidate))
            {
                var detected = ResolveMSVCFromVsInstallPath(vsDirCandidate);
                if (ValidateMSVC(detected)) return detected!;
            }
        }

        // Priority 5: Fixed drive roots scan (FINAL FALLBACK ONLY)
        Log.Information("Attempting fallback MSVC detection via drive scanning as last resort");
        try
        {
            foreach (var drive in DriveInfo.GetDrives().Where(d => d.DriveType == DriveType.Fixed && d.IsReady))
            {
                var root = drive.RootDirectory.FullName;
                var commonVcRoots = new[]
                {
                    Path.Combine(root, "vs"),
                    Path.Combine(root, "Microsoft Visual Studio"),
                };

                foreach (var vcRoot in commonVcRoots)
                {
                    if (Directory.Exists(vcRoot))
                    {
                        var detectedRoot = ResolveMSVCFromVsInstallPath(vcRoot);
                        if (ValidateMSVC(detectedRoot)) return detectedRoot!;
                    }
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
    /// Finds vswhere.exe in standard installer paths or PATH without scanning drive roots.
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

        if (TryFindCompiler("vswhere.exe", out var pathInPath) && !string.IsNullOrEmpty(pathInPath))
            candidates.Add(pathInPath);
        if (TryFindCompiler("vswhere", out var pathInPathNoExt) && !string.IsNullOrEmpty(pathInPathNoExt))
            candidates.Add(pathInPathNoExt);

        foreach (var c in candidates)
        {
            if (File.Exists(c)) return c;
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

        return new DetectedCompiler
        {
            Type = CompilerType.MSVC,
            Path = clPath,
            Version = ExtractMSVCVersion(latestVersion),
            IncludePath = Path.Combine(latestVersion, "include"),
            LibraryPath = Path.Combine(latestVersion, "lib", hostArch),
            VcVarsAllPath = vcvarsallPath,
            VsInstallPath = vsPath,
            HostArchitecture = hostArch,
            TargetArchitecture = hostArch,
            IsValid = true
        };
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
                VsInstallPath = vsInstallDir?.FullName,
                HostArchitecture = hostArch,
                TargetArchitecture = hostArch,
                IsValid = true
            };
        }
        catch
        {
            return null;
        }
    }

    /// <summary>
    /// Loads persistent toolchain cache file.
    /// </summary>
    private static DetectedCompiler? LoadPersistentCache()
    {
        try
        {
            var projectDb = IgniteBT.Core.Launcher.BuildEnvironment.ResolveProjectDatabaseDirectory();
            var cachePath = Path.Combine(projectDb, CacheFileName);
            if (!File.Exists(cachePath)) return null;

            var json = File.ReadAllText(cachePath);
            var cached = JsonSerializer.Deserialize<DetectedCompiler>(json, new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true
            });

            return cached;
        }
        catch
        {
            return null;
        }
    }

    /// <summary>
    /// Saves persistent toolchain cache file.
    /// </summary>
    private static void SavePersistentCache(DetectedCompiler compiler)
    {
        try
        {
            var projectDb = IgniteBT.Core.Launcher.BuildEnvironment.ResolveProjectDatabaseDirectory();
            Directory.CreateDirectory(projectDb);
            var cachePath = Path.Combine(projectDb, CacheFileName);
            var json = JsonSerializer.Serialize(compiler, new JsonSerializerOptions
            {
                WriteIndented = true,
                PropertyNamingPolicy = JsonNamingPolicy.CamelCase
            });
            File.WriteAllText(cachePath, json);
        }
        catch (Exception ex)
        {
            Log.Debug(ex, "Failed to save persistent toolchain cache");
        }
    }

    /// <summary>
    /// Detects GCC or Clang on Linux.
    /// </summary>
    private static DetectedCompiler DetectGCCOrClangLinux()
    {
        Log.Information("Detecting compiler on Linux");

        if (TryFindCompiler("clang", out var clangPath) && clangPath != null)
        {
            Log.Information("Found clang at {Path}", clangPath);
            return new DetectedCompiler
            {
                Type = CompilerType.Clang,
                Path = clangPath,
                Version = GetCompilerVersion(clangPath),
                IsValid = true
            };
        }

        if (TryFindCompiler("gcc", out var gccPath) && gccPath != null)
        {
            Log.Information("Found gcc at {Path}", gccPath);
            return new DetectedCompiler
            {
                Type = CompilerType.GCC,
                Path = gccPath,
                Version = GetCompilerVersion(gccPath),
                IsValid = true
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
                Version = GetCompilerVersion(clangPath),
                IsValid = true
            };
        }

        Log.Warning("Clang not found on macOS");
        return new DetectedCompiler { Type = CompilerType.None, Path = string.Empty };
    }

    /// <summary>
    /// Tries to find a compiler executable in system PATH.
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
    /// Extracts MSVC version from version directory path.
    /// </summary>
    private static string ExtractMSVCVersion(string versionPath)
    {
        return Path.GetFileName(versionPath);
    }

    /// <summary>
    /// Gets compiler version by running it with --version flag.
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

            var firstLine = output.Split('\n').FirstOrDefault();
            return firstLine ?? "Unknown";
        }
        catch
        {
            return "Unknown";
        }
    }

    /// <summary>
    /// Sets up MSVC environment by running vcvarsall.bat and returning environment variables.
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
/// Represents a detected compiler toolchain description.
/// </summary>
public class DetectedCompiler
{
    public CompilerType Type { get; set; }
    public string Path { get; set; } = string.Empty;
    public string Version { get; set; } = string.Empty;
    public string? IncludePath { get; set; }
    public string? LibraryPath { get; set; }
    public string? VcVarsAllPath { get; set; }
    public string? VsInstallPath { get; set; }
    public string HostArchitecture { get; set; } = "x64";
    public string TargetArchitecture { get; set; } = "x64";
    public bool IsValid { get; set; } = false;
    public Dictionary<string, string> EnvironmentVariables { get; set; } = new();
    public DateTime LastValidatedUtc { get; set; } = DateTime.UtcNow;
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
