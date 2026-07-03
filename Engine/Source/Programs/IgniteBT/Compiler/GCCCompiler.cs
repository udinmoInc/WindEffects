using System.Diagnostics;
using Serilog;
using System.Text.RegularExpressions;

namespace IgniteBT.Compiler;

/// <summary>
/// GCC compiler implementation (Linux/macOS).
/// </summary>
public class GCCCompiler : ICompiler
{
    private string? _version;
    private string? _executablePath;
    private TargetPlatform _platform;

    public string Name => "GCC";
    public string ExecutablePath => _executablePath ?? FindExecutable();
    public string Version => _version ?? string.Empty;
    public TargetPlatform Platform => _platform;

    public GCCCompiler(TargetPlatform platform = TargetPlatform.Unknown)
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

            // Parse version from output (e.g., "g++ (GCC) 11.3.0")
            var versionMatch = Regex.Match(output, @"g\+\+.*?(\d+\.\d+\.\d+)");
            if (versionMatch.Success)
            {
                _version = versionMatch.Groups[1].Value;
                return _version;
            }

            return "Unknown";
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to detect GCC version");
            return "Unknown";
        }
    }

    public async Task<CompilationResult> CompileAsync(CompilerOptions options)
    {
        var result = new CompilationResult
        {
            OutputFile = options.OutputFile
        };

        var stopwatch = Stopwatch.StartNew();

        try
        {
            var arguments = BuildCompileArguments(options);

            Log.Information("Compiling {SourceFile} with GCC", options.SourceFile);
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
                result.StandardError = "Failed to start compiler process";
                return result;
            }

            result.StandardOutput = await process.StandardOutput.ReadToEndAsync();
            result.StandardError = await process.StandardError.ReadToEndAsync();
            await process.WaitForExitAsync();

            result.ExitCode = process.ExitCode;
            result.Success = process.ExitCode == 0;

            // Parse diagnostics
            result.Diagnostics = ParseDiagnostics(result.StandardError, options.SourceFile);

            stopwatch.Stop();
            result.CompilationTimeMs = stopwatch.ElapsedMilliseconds;

            if (result.Success)
            {
                Log.Information("Compiled {SourceFile} in {Time}ms", options.SourceFile, result.CompilationTimeMs);
            }
            else
            {
                Log.Error("Failed to compile {SourceFile}: {Error}", options.SourceFile, result.StandardError);
            }

            return result;
        }
        catch (Exception ex)
        {
            stopwatch.Stop();
            result.CompilationTimeMs = stopwatch.ElapsedMilliseconds;
            result.Success = false;
            result.ExitCode = -1;
            result.StandardError = ex.Message;
            Log.Error(ex, "Exception during compilation of {SourceFile}", options.SourceFile);
            return result;
        }
    }

    public async Task<CompilationResult> GeneratePrecompiledHeaderAsync(CompilerOptions options)
    {
        options.IsPrecompiledHeader = true;
        return await CompileAsync(options);
    }

    public List<string> GetDefaultFlags(BuildConfiguration configuration)
    {
        var flags = new List<string>
        {
            "-fno-exceptions",
            "-fno-rtti",
            "-fno-threadsafe-statics",
            "-fvisibility-inlines-hidden"
        };

        flags.AddRange(GetWarningFlags());
        flags.AddRange(GetOptimizationFlags(configuration));

        return flags;
    }

    public List<string> GetOptimizationFlags(BuildConfiguration configuration)
    {
        return configuration switch
        {
            BuildConfiguration.Debug => new List<string> { "-O0", "-g" },
            BuildConfiguration.Development => new List<string> { "-O2", "-g" },
            BuildConfiguration.Profile => new List<string> { "-O2", "-g", "-fprofile-generate" },
            BuildConfiguration.Shipping => new List<string> { "-O3", "-flto", "-DNDEBUG" },
            _ => new List<string>()
        };
    }

    public List<string> GetWarningFlags()
    {
        return new List<string>
        {
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Wno-unused-parameter"
        };
    }

    private string BuildCompileArguments(CompilerOptions options)
    {
        var args = new List<string>();

        // Output
        args.Add("-o");
        args.Add(options.OutputFile);

        // Compile only (no linking)
        args.Add("-c");

        // Configuration-specific flags
        args.AddRange(GetDefaultFlags(options.Configuration));

        // C++ standard
        args.Add($"-std={options.CppStandard}");

        // Platform-specific flags
        if (_platform == TargetPlatform.Linux)
        {
            args.Add("-fPIC");
            args.Add("-pthread");
        }
        else if (_platform == TargetPlatform.MacOS)
        {
            args.Add("-fPIC");
        }

        // Include directories
        foreach (var includeDir in options.IncludeDirectories)
        {
            args.Add($"-I{includeDir}");
        }

        // Definitions
        foreach (var definition in options.Definitions)
        {
            args.Add($"-D{definition}");
        }

        // Precompiled header
        if (options.IsPrecompiledHeader)
        {
            args.Add($"-x{GetPchLanguage()}");
        }
        else if (!string.IsNullOrEmpty(options.PrecompiledHeader))
        {
            args.Add($"-include{options.PrecompiledHeader}");
        }

        // Additional flags
        args.AddRange(options.AdditionalFlags);

        // Source file
        args.Add(options.SourceFile);

        return string.Join(" ", args);
    }

    private string FindExecutable()
    {
        if (!string.IsNullOrEmpty(_executablePath))
        {
            return _executablePath;
        }

        var exeName = _platform switch
        {
            TargetPlatform.Windows => "g++.exe",
            _ => "g++"
        };

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
            TargetPlatform.Windows => new[]
            {
                @"C:\MinGW\bin\g++.exe",
                @"C:\MinGW64\bin\g++.exe",
                @"C:\TDM-GCC-64\bin\g++.exe",
                @"C:\msys64\mingw64\bin\g++.exe"
            },
            TargetPlatform.Linux => new[]
            {
                "/usr/bin/g++",
                "/usr/local/bin/g++"
            },
            TargetPlatform.MacOS => new[]
            {
                "/usr/bin/g++",
                "/usr/local/bin/g++",
                "/opt/homebrew/bin/g++"
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

    private string GetPchLanguage()
    {
        return "c++-header";
    }

    private List<CompilerDiagnostic> ParseDiagnostics(string output, string sourceFile)
    {
        var diagnostics = new List<CompilerDiagnostic>();

        // GCC error format: filename:line:column: error/warning: message
        var pattern = @"^(.+):(\d+):(\d+):\s+(error|warning):\s+(.+)$";
        var regex = new Regex(pattern, RegexOptions.Multiline);

        foreach (Match match in regex.Matches(output))
        {
            var diagnostic = new CompilerDiagnostic
            {
                FilePath = match.Groups[1].Value,
                Line = int.Parse(match.Groups[2].Value),
                Column = int.Parse(match.Groups[3].Value),
                Severity = match.Groups[4].Value == "error" ? DiagnosticSeverity.Error : DiagnosticSeverity.Warning,
                Message = match.Groups[5].Value
            };
            diagnostics.Add(diagnostic);
        }

        return diagnostics;
    }
}
