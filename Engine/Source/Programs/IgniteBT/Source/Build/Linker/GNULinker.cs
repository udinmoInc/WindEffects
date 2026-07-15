using System.Diagnostics;
using Serilog;
using IgniteBT.Build.Compiler;

namespace IgniteBT.Build.Linker;

/// <summary>
/// GNU linker implementation (ld for Linux/macOS).
/// </summary>
public class GNULinker : ILinker
{
    private string? _version;
    private string? _executablePath;
    private TargetPlatform _platform;

    public string Name => "GNU ld";
    public string ExecutablePath => _executablePath ?? FindExecutable();
    public string Version => _version ?? string.Empty;
    public TargetPlatform Platform => _platform;

    public GNULinker(TargetPlatform platform = TargetPlatform.Unknown)
    {
        _platform = platform;
        if (platform == TargetPlatform.Unknown)
        {
            _platform = DetectPlatform();
        }
    }

    public bool IsAvailable()
    {
        var exePath = FindExecutable();
        return !string.IsNullOrEmpty(exePath) && File.Exists(exePath);
    }

    public async Task<string> DetectVersionAsync()
    {
        try
        {
            var exePath = ExecutablePath;
            if (string.IsNullOrEmpty(exePath) || !File.Exists(exePath))
            {
                return "Not available";
            }

            var startInfo = new ProcessStartInfo
            {
                FileName = exePath,
                Arguments = "--version",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var process = Process.Start(startInfo);
            if (process == null)
            {
                return "Unknown";
            }

            var output = await process.StandardOutput.ReadToEndAsync();
            await process.WaitForExitAsync();

            // Parse version from output
            var versionMatch = System.Text.RegularExpressions.Regex.Match(output, @"GNU ld (\d+\.\d+)");
            if (versionMatch.Success)
            {
                _version = versionMatch.Groups[1].Value;
                return _version;
            }

            return "Unknown";
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to detect GNU ld version");
            return "Unknown";
        }
    }

    public async Task<LinkResult> LinkAsync(LinkerOptions options)
    {
        var result = new LinkResult
        {
            OutputFile = options.OutputFile
        };

        var stopwatch = Stopwatch.StartNew();

        try
        {
            var arguments = BuildLinkArguments(options);

            Log.Information("Linking {TargetType} to {OutputFile} with GNU ld", options.TargetType, options.OutputFile);
            Log.Debug("Arguments: {Arguments}", arguments);

            var startInfo = new ProcessStartInfo
            {
                FileName = ExecutablePath,
                Arguments = arguments,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
                WorkingDirectory = options.WorkingDirectory
            };

            using var process = Process.Start(startInfo);
            if (process == null)
            {
                result.Success = false;
                result.ExitCode = -1;
                result.StandardError = "Failed to start linker process";
                return result;
            }

            result.StandardOutput = await process.StandardOutput.ReadToEndAsync();
            result.StandardError = await process.StandardError.ReadToEndAsync();
            await process.WaitForExitAsync();

            result.ExitCode = process.ExitCode;
            result.Success = process.ExitCode == 0;

            stopwatch.Stop();
            result.LinkTimeMs = stopwatch.ElapsedMilliseconds;

            if (result.Success)
            {
                Log.Information("Linked {OutputFile} in {Time}ms", options.OutputFile, result.LinkTimeMs);
            }
            else
            {
                Log.Error("Failed to link {OutputFile}: {Error}", options.OutputFile, result.StandardError);
            }

            return result;
        }
        catch (Exception ex)
        {
            stopwatch.Stop();
            result.LinkTimeMs = stopwatch.ElapsedMilliseconds;
            result.Success = false;
            result.ExitCode = -1;
            result.StandardError = ex.Message;
            Log.Error(ex, "Exception during linking of {OutputFile}", options.OutputFile);
            return result;
        }
    }

    public async Task<LinkResult> CreateStaticLibraryAsync(LinkerOptions options)
    {
        // Use ar for static libraries
        var arExe = FindArExecutable();
        if (string.IsNullOrEmpty(arExe))
        {
            return new LinkResult
            {
                Success = false,
                StandardError = "ar not found"
            };
        }

        var result = new LinkResult
        {
            OutputFile = options.OutputFile
        };

        var stopwatch = Stopwatch.StartNew();

        try
        {
            var arguments = BuildArArguments(options);

            Log.Information("Creating static library {OutputFile} with ar", options.OutputFile);
            Log.Debug("Arguments: {Arguments}", arguments);

            var startInfo = new ProcessStartInfo
            {
                FileName = arExe,
                Arguments = arguments,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
                WorkingDirectory = options.WorkingDirectory
            };

            using var process = Process.Start(startInfo);
            if (process == null)
            {
                result.Success = false;
                result.ExitCode = -1;
                result.StandardError = "Failed to start ar process";
                return result;
            }

            result.StandardOutput = await process.StandardOutput.ReadToEndAsync();
            result.StandardError = await process.StandardError.ReadToEndAsync();
            await process.WaitForExitAsync();

            result.ExitCode = process.ExitCode;
            result.Success = process.ExitCode == 0;

            stopwatch.Stop();
            result.LinkTimeMs = stopwatch.ElapsedMilliseconds;

            if (result.Success)
            {
                Log.Information("Created static library {OutputFile} in {Time}ms", options.OutputFile, result.LinkTimeMs);
            }
            else
            {
                Log.Error("Failed to create static library {OutputFile}: {Error}", options.OutputFile, result.StandardError);
            }

            return result;
        }
        catch (Exception ex)
        {
            stopwatch.Stop();
            result.LinkTimeMs = stopwatch.ElapsedMilliseconds;
            result.Success = false;
            result.ExitCode = -1;
            result.StandardError = ex.Message;
            Log.Error(ex, "Exception during static library creation of {OutputFile}", options.OutputFile);
            return result;
        }
    }

    public List<string> GetDefaultFlags(BuildConfiguration configuration, LinkTargetType targetType)
    {
        var flags = new List<string>
        {
            "--no-undefined",
            "--as-needed"
        };

        flags.AddRange(GetSubsystemFlags(targetType));

        return flags;
    }

    public List<string> GetSubsystemFlags(LinkTargetType targetType)
    {
        return targetType switch
        {
            LinkTargetType.Executable => new List<string>(),
            LinkTargetType.SharedLibrary => new List<string> { "-shared", "-fPIC" },
            LinkTargetType.StaticLibrary => new List<string>(),
            _ => new List<string>()
        };
    }

    private string BuildLinkArguments(LinkerOptions options)
    {
        var args = new List<string>();

        // Output
        args.Add("-o");
        args.Add(options.OutputFile);

        // Configuration-specific flags
        args.AddRange(GetDefaultFlags(options.Configuration, options.TargetType));

        // Debug info
        if (options.GenerateDebugInfo)
        {
            args.Add("-g");
        }

        // Library directories
        foreach (var libDir in options.LibraryDirectories)
        {
            args.Add($"-L{libDir}");
        }

        // Object files
        foreach (var objFile in options.ObjectFiles)
        {
            args.Add(objFile);
        }

        // Libraries
        foreach (var lib in options.Libraries)
        {
            // Remove .lib/.a extension for Unix-style linking
            var libName = lib.EndsWith(".lib") || lib.EndsWith(".a") 
                ? Path.GetFileNameWithoutExtension(lib)
                : lib;
            args.Add($"-l{libName}");
        }

        // Additional flags
        args.AddRange(options.AdditionalFlags);

        return string.Join(" ", args);
    }

    private string BuildArArguments(LinkerOptions options)
    {
        var args = new List<string>
        {
            "rcs",
            options.OutputFile
        };

        // Object files
        foreach (var objFile in options.ObjectFiles)
        {
            args.Add(objFile);
        }

        return string.Join(" ", args);
    }

    private string FindExecutable()
    {
        if (!string.IsNullOrEmpty(_executablePath))
        {
            return _executablePath;
        }

        var exeName = "ld";

        // Try PATH first
        var inPath = FindInPath(exeName);
        if (!string.IsNullOrEmpty(inPath))
        {
            _executablePath = inPath;
            return inPath;
        }

        // Common installation paths
        var commonPaths = _platform switch
        {
            TargetPlatform.Linux => new[]
            {
                "/usr/bin/ld",
                "/usr/local/bin/ld",
                "/usr/bin/ld.gold",
                "/usr/bin/ld.bfd"
            },
            TargetPlatform.MacOS => new[]
            {
                // macOS uses ld64, which is different from GNU ld
                // This might not work as expected on macOS
                "/usr/bin/ld"
            },
            _ => Array.Empty<string>()
        };

        foreach (var path in commonPaths)
        {
            if (File.Exists(path))
            {
                _executablePath = path;
                return path;
            }
        }

        return string.Empty;
    }

    private string FindArExecutable()
    {
        var exeName = "ar";

        var inPath = FindInPath(exeName);
        if (!string.IsNullOrEmpty(inPath))
        {
            return inPath;
        }

        var commonPaths = _platform switch
        {
            TargetPlatform.Linux => new[]
            {
                "/usr/bin/ar",
                "/usr/local/bin/ar"
            },
            TargetPlatform.MacOS => new[]
            {
                "/usr/bin/ar",
                "/usr/local/bin/ar",
                "/opt/homebrew/bin/ar"
            },
            _ => Array.Empty<string>()
        };

        foreach (var path in commonPaths)
        {
            if (File.Exists(path))
            {
                return path;
            }
        }

        return string.Empty;
    }

    private string? FindInPath(string exeName)
    {
        var pathEnv = Environment.GetEnvironmentVariable("PATH");
        if (string.IsNullOrEmpty(pathEnv))
            return null;

        foreach (var dir in pathEnv.Split(Path.PathSeparator))
        {
            var fullPath = Path.Combine(dir, exeName);
            if (File.Exists(fullPath))
                return fullPath;
        }

        return null;
    }

    private TargetPlatform DetectPlatform()
    {
        if (OperatingSystem.IsWindows())
            return TargetPlatform.Windows;
        if (OperatingSystem.IsLinux())
            return TargetPlatform.Linux;
        if (OperatingSystem.IsMacOS())
            return TargetPlatform.MacOS;
        return TargetPlatform.Unknown;
    }
}
