using System.Diagnostics;
using Serilog;
using System.Text.RegularExpressions;
using IgniteBT.Build.Toolchain;

namespace IgniteBT.Build.Compiler;

/// <summary>
/// Microsoft Visual C++ compiler implementation.
/// </summary>
public class MSVCCompiler : ICompiler
{
    private string? _version;
    private string? _executablePath;
    private Dictionary<string, string>? _environmentVariables;
    private string? _vcVarsAllPath;

    public string Name => "MSVC";
    public string ExecutablePath => _executablePath ?? FindExecutable();
    public string Version => _version ?? string.Empty;
    public TargetPlatform Platform => TargetPlatform.Windows;

    /// <summary>
    /// Sets the executable path explicitly (e.g., from ToolchainDetector).
    /// </summary>
    public void SetExecutablePath(string path)
    {
        _executablePath = path;
    }

    /// <summary>
    /// Sets the environment variables from vcvarsall.bat.
    /// </summary>
    public void SetEnvironmentVariables(Dictionary<string, string> envVars)
    {
        _environmentVariables = envVars;
    }

    /// <summary>
    /// Sets the vcvarsall.bat path.
    /// </summary>
    public void SetVcVarsAllPath(string path)
    {
        _vcVarsAllPath = path;
    }

    public bool IsAvailable()
    {
        var exePath = ExecutablePath;
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
                Arguments = "/?",
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
            var versionMatch = Regex.Match(output, @"Compiler Version (\d+\.\d+\.\d+)");
            if (versionMatch.Success)
            {
                _version = versionMatch.Groups[1].Value;
                return _version;
            }

            return "Unknown";
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to detect MSVC version");
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

            Log.Information("Compiling {SourceFile} with MSVC", options.SourceFile);
            Log.Information("Full command: {Exe} {Args}", ExecutablePath, arguments);

            var cachedEnv = _environmentVariables
                ?? VcEnvironmentCache.LoadOrCapture(_vcVarsAllPath);

            var startInfo = new ProcessStartInfo
            {
                FileName = ExecutablePath,
                Arguments = arguments,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
                WorkingDirectory = string.IsNullOrEmpty(options.WorkingDirectory)
                    ? Environment.CurrentDirectory
                    : options.WorkingDirectory
            };
            VcEnvironmentCache.ApplyToProcessStartInfo(startInfo, cachedEnv);

            using var process = Process.Start(startInfo);
            if (process == null)
            {
                result.Success = false;
                result.ExitCode = -1;
                result.StandardError = "Failed to start compiler process";
                Log.Error("Failed to start compiler process");
                return result;
            }

            var stdoutTask = process.StandardOutput.ReadToEndAsync();
            var stderrTask = process.StandardError.ReadToEndAsync();
            await Task.WhenAll(stdoutTask, stderrTask, process.WaitForExitAsync());

            result.StandardOutput = await stdoutTask;
            result.StandardError = await stderrTask;
            result.ExitCode = process.ExitCode;
            result.Success = process.ExitCode == 0;

            Log.Debug("Compiler process exited with code: {ExitCode}", process.ExitCode);

            // Log output for debugging
            if (process.ExitCode != 0)
            {
                Log.Information("Compiler stdout: {Output}", result.StandardOutput);
            }
            if (!string.IsNullOrWhiteSpace(result.StandardError))
            {
                Log.Information("Compiler stderr: {Error}", result.StandardError);
            }

            if (string.IsNullOrEmpty(result.StandardOutput) && string.IsNullOrEmpty(result.StandardError))
            {
                Log.Warning("Compiler produced no output (stdout or stderr)");
            }

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
            "/nologo",
            "/EHsc",
            "/utf-8",
            "/Zc:__cplusplus",
            "/Zc:preprocessor",
            "/DWIN32_LEAN_AND_MEAN",
            "/DNOMINMAX",
            "/FS"
        };

        flags.AddRange(GetWarningFlags());
        flags.AddRange(GetOptimizationFlags(configuration));

        return flags;
    }

    public List<string> GetOptimizationFlags(BuildConfiguration configuration)
    {
        return configuration switch
        {
            BuildConfiguration.Debug => new List<string> { "/Od", "/RTC1" },
            BuildConfiguration.Development => new List<string> { "/O1", "/Ob1" },
            BuildConfiguration.Profile => new List<string> { "/O2", "/GT" },
            BuildConfiguration.Shipping => new List<string> { "/O2", "/Oi", "/Ot", "/GS-", "/GL" },
            _ => new List<string>()
        };
    }

    public List<string> GetWarningFlags()
    {
        return new List<string>
        {
            "/W4",
            "/WX",
            "/wd4100",
            "/wd4189",
            "/wd4702",
            "/wd4996",
            "/wd4073"
        };
    }

    public string BuildArguments(CompilerOptions options) => BuildCompileArguments(options);

    private string BuildCompileArguments(CompilerOptions options)
    {
        var args = new List<string>();

        // Compile only (no linking)
        args.Add("/c");

        // Output
        args.Add($"/Fo\"{options.OutputFile}\"");

        // Configuration-specific flags
        args.AddRange(GetDefaultFlags(options.Configuration));

        // Debug info (/Z7 embeds debug symbols into object file, avoiding mspdbcore.dll PDB locking overhead)
        if (options.GenerateDebugInfo)
        {
            args.Add("/Z7");
            if (options.Configuration == BuildConfiguration.Debug)
            {
                args.Add("/MD");
                args.Add("/D_ITERATOR_DEBUG_LEVEL=0");
            }
            else
            {
                args.Add("/MD");
            }
        }
        else
        {
            args.Add("/MD");
        }

        // C++ standard (MSVC uses /std:c++17, /std:c++20, /std:c++latest, etc.)
        var cppStandard = options.CppStandard.ToLowerInvariant();
        var msvcStandard = cppStandard switch
        {
            "c++23" or "23" => "c++latest",
            _ when cppStandard.StartsWith("c++", StringComparison.Ordinal) => cppStandard,
            _ => $"c++{cppStandard}"
        };
        args.Add($"/std:{msvcStandard}");

        // Include directories
        foreach (var includeDir in options.IncludeDirectories)
        {
            args.Add($"/I\"{includeDir}\"");
        }

        // Add MSVC environment include paths if available
        if (_environmentVariables != null && _environmentVariables.ContainsKey("INCLUDE"))
        {
            var includePaths = _environmentVariables["INCLUDE"].Split(';', StringSplitOptions.RemoveEmptyEntries);
            foreach (var path in includePaths)
            {
                args.Add($"/I\"{path}\"");
            }
        }
        else
        {
            // Automatically resolve MSVC core headers if running from a local toolchain path
            var exeDir = Path.GetDirectoryName(ExecutablePath);
            if (!string.IsNullOrEmpty(exeDir))
            {
                var msvcRoot = Path.GetFullPath(Path.Combine(exeDir, "..", "..", ".."));
                var msvcIncludeDir = Path.Combine(msvcRoot, "include");
                if (Directory.Exists(msvcIncludeDir))
                {
                    args.Add($"/I\"{msvcIncludeDir}\"");
                }
            }
        }

        // Definitions
        foreach (var definition in options.Definitions)
        {
            args.Add($"/D{definition}");
        }

        // Precompiled header
        if (options.IsPrecompiledHeader)
        {
            var hName = Path.GetFileName(options.PrecompiledHeader);
            args.Add($"/Yc\"{hName}\"");
            var pchPath = options.OutputFile.EndsWith(".obj", StringComparison.OrdinalIgnoreCase)
                ? options.OutputFile[..^4]
                : options.OutputFile;
            args.Add($"/Fp\"{pchPath}\"");
        }
        else if (!string.IsNullOrEmpty(options.PrecompiledHeader))
        {
            var hName = Path.GetFileName(options.PrecompiledHeader);
            args.Add($"/Yu\"{hName}\"");
        }

        // Additional flags
        args.AddRange(options.AdditionalFlags);

        // Source file
        args.Add($"\"{options.SourceFile}\"");

        return string.Join(" ", args);
    }

    private string FindExecutable()
    {
        if (!string.IsNullOrEmpty(_executablePath))
        {
            return _executablePath;
        }

        var detected = ToolchainDetector.DetectCompiler();
        if (detected.Type == CompilerType.MSVC && !string.IsNullOrEmpty(detected.Path))
        {
            _executablePath = detected.Path;
            if (!string.IsNullOrEmpty(detected.VcVarsAllPath))
            {
                _vcVarsAllPath = detected.VcVarsAllPath;
            }
            return _executablePath;
        }

        return string.Empty;
    }

    private List<CompilerDiagnostic> ParseDiagnostics(string output, string sourceFile)
    {
        var diagnostics = new List<CompilerDiagnostic>();

        // MSVC error format: filename(line,column): error/warning CXXXX: message
        var pattern = @"^(.+)\((\d+),(\d+)\):\s+(error|warning)\s+(C\d+):\s+(.+)$";
        var regex = new Regex(pattern, RegexOptions.Multiline);

        foreach (Match match in regex.Matches(output))
        {
            var diagnostic = new CompilerDiagnostic
            {
                FilePath = match.Groups[1].Value,
                Line = int.Parse(match.Groups[2].Value),
                Column = int.Parse(match.Groups[3].Value),
                Severity = match.Groups[4].Value == "error" ? DiagnosticSeverity.Error : DiagnosticSeverity.Warning,
                Code = match.Groups[5].Value,
                Message = match.Groups[6].Value
            };
            diagnostics.Add(diagnostic);
        }

        return diagnostics;
    }
}
