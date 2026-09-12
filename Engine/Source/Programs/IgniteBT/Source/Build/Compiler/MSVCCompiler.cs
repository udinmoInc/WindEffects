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
            Log.Debug("Working Directory: {WorkingDir}", options.WorkingDirectory);
            Log.Debug("Source file exists: {Exists}", File.Exists(options.SourceFile));

            ProcessStartInfo startInfo;

            // If we have vcvarsall.bat, run through cmd.exe with it to set up the environment
            if (!string.IsNullOrEmpty(_vcVarsAllPath))
            {
                Log.Debug("vcvarsall.bat path: {VcVarsPath}", _vcVarsAllPath);
                Log.Debug("vcvarsall.bat exists: {Exists}", File.Exists(_vcVarsAllPath));

                var tempBatPath = Path.Combine(Path.GetTempPath(), $"ignitebt_compile_{Guid.NewGuid()}.bat");

                var batchContent = $@"@echo off
echo Running vcvarsall.bat...
call ""{_vcVarsAllPath}"" x64
echo Running compiler...
""{ExecutablePath}"" {arguments}
echo Compiler exit code: %ERRORLEVEL%
exit /b %ERRORLEVEL%
";
                File.WriteAllText(tempBatPath, batchContent);

                Log.Debug("Batch file created at: {BatPath}", tempBatPath);

                startInfo = new ProcessStartInfo
                {
                    FileName = "cmd.exe",
                    Arguments = $"/c \"{tempBatPath}\"",
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    WorkingDirectory = options.WorkingDirectory
                };

                Log.Debug("Running compiler through batch file");

                using var batchProcess = Process.Start(startInfo);
                if (batchProcess == null)
                {
                    result.Success = false;
                    result.ExitCode = -1;
                    result.StandardError = "Failed to start compiler process";
                    Log.Error("Failed to start compiler process");
                    return result;
                }

                result.StandardOutput = await batchProcess.StandardOutput.ReadToEndAsync();
                result.StandardError = await batchProcess.StandardError.ReadToEndAsync();
                await batchProcess.WaitForExitAsync();

                result.ExitCode = batchProcess.ExitCode;
                result.Success = batchProcess.ExitCode == 0;

                // Clean up temp file
                try
                {
                    if (File.Exists(tempBatPath))
                    {
                        File.Delete(tempBatPath);
                    }
                }
                catch
                {
                    // Ignore cleanup errors
                }

                stopwatch.Stop();
                result.CompilationTimeMs = stopwatch.ElapsedMilliseconds;

                Log.Information("Compiler process exited with code: {ExitCode}", batchProcess.ExitCode);

                if (!string.IsNullOrEmpty(result.StandardOutput))
                {
                    Log.Information("Compiler stdout:\n{Output}", result.StandardOutput);
                }
                if (!string.IsNullOrEmpty(result.StandardError))
                {
                    Log.Error("Compiler stderr:\n{Error}", result.StandardError);
                }

                if (result.Success)
                {
                    Log.Information("Compiled {SourceFile} in {Time}ms", options.SourceFile, result.CompilationTimeMs);
                }
                else
                {
                    Log.Error("Compilation failed with exit code {ExitCode}", batchProcess.ExitCode);
                }

                result.Diagnostics = ParseDiagnostics(result.StandardError, options.SourceFile);
                return result;
            }
            else
            {
                startInfo = new ProcessStartInfo
                {
                    FileName = ExecutablePath,
                    Arguments = arguments,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    WorkingDirectory = options.WorkingDirectory
                };
            }

            using var process = Process.Start(startInfo);
            if (process == null)
            {
                result.Success = false;
                result.ExitCode = -1;
                result.StandardError = "Failed to start compiler process";
                Log.Error("Failed to start compiler process");
                return result;
            }

            Log.Debug("Compiler process started with ID: {ProcessId}", process.Id);

            result.StandardOutput = await process.StandardOutput.ReadToEndAsync();
            result.StandardError = await process.StandardError.ReadToEndAsync();
            await process.WaitForExitAsync();

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
            "/FS"  // Force file system locking for PDB files (required for parallel compilation)
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
            BuildConfiguration.Development => new List<string> { "/O2" },
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

        // Debug info
        if (options.GenerateDebugInfo)
        {
            args.Add("/Zi");
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
            args.Add($"/Yc\"{options.PrecompiledHeader}\"");
        }
        else if (!string.IsNullOrEmpty(options.PrecompiledHeader))
        {
            args.Add($"/Yu\"{options.PrecompiledHeader}\"");
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
