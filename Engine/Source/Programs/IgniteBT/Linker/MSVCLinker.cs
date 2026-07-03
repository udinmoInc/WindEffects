using System.Diagnostics;
using Serilog;
using IgniteBT.Compiler;

namespace IgniteBT.Linker;

/// <summary>
/// Microsoft Visual C++ linker implementation.
/// </summary>
public class MSVCLinker : ILinker
{
    private string? _version;
    private string? _executablePath;
    private Dictionary<string, string>? _environmentVariables;
    private string? _vcVarsAllPath;

    public string Name => "MSVC LINK";
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
            var versionMatch = System.Text.RegularExpressions.Regex.Match(output, @"Microsoft \(R\) Incremental Linker Version (\d+\.\d+\.\d+)");
            if (versionMatch.Success)
            {
                _version = versionMatch.Groups[1].Value;
                return _version;
            }

            return "Unknown";
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to detect MSVC linker version");
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
            if (options.ExportAllSymbols && options.TargetType == LinkTargetType.SharedLibrary)
            {
                var definitionFile = await GenerateExportDefinitionFileAsync(options);
                if (!string.IsNullOrEmpty(definitionFile))
                {
                    options.DefinitionFile = definitionFile;
                }
            }

            var arguments = BuildLinkArguments(options);

            Log.Information("Linking {TargetType} to {OutputFile} with MSVC LINK", options.TargetType, options.OutputFile);
            Log.Debug("Linker executable: {ExePath}", ExecutablePath);
            Log.Debug("Linker executable exists: {Exists}", File.Exists(ExecutablePath));
            Log.Debug("Arguments: {Arguments}", arguments);
            Log.Debug("Working directory: {WorkingDir}", options.WorkingDirectory);

            ProcessStartInfo startInfo;
            string? tempBatPath = null;

            // If we have vcvarsall.bat, run through cmd.exe with it to set up the environment
            if (!string.IsNullOrEmpty(_vcVarsAllPath))
            {
                Log.Debug("vcvarsall.bat path: {VcVarsPath}", _vcVarsAllPath);
                Log.Debug("vcvarsall.bat exists: {Exists}", File.Exists(_vcVarsAllPath));
                
                tempBatPath = Path.Combine(Path.GetTempPath(), $"ignitebt_link_{Guid.NewGuid()}.bat");
                
                var batchContent = $@"@echo off
echo Running vcvarsall.bat...
call ""{_vcVarsAllPath}"" x64
echo Running linker...
""{ExecutablePath}"" {arguments}
echo Linker exit code: %ERRORLEVEL%
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
                
                Log.Debug("Running linker through batch file");
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
                result.StandardError = "Failed to start linker process";
                Log.Error("Failed to start linker process");
                return result;
            }

            result.StandardOutput = await process.StandardOutput.ReadToEndAsync();
            result.StandardError = await process.StandardError.ReadToEndAsync();
            await process.WaitForExitAsync();

            result.ExitCode = process.ExitCode;
            result.Success = process.ExitCode == 0;

            Log.Debug("Linker exit code: {ExitCode}", process.ExitCode);
            if (!string.IsNullOrEmpty(result.StandardOutput))
            {
                Log.Information("Linker stdout:\n{Output}", result.StandardOutput);
            }
            if (!string.IsNullOrEmpty(result.StandardError))
            {
                Log.Error("Linker stderr:\n{Error}", result.StandardError);
            }

            // Clean up temp batch file if we used one
            if (!string.IsNullOrEmpty(tempBatPath) && File.Exists(tempBatPath))
            {
                try
                {
                    File.Delete(tempBatPath);
                    Log.Debug("Cleaned up temp batch file: {BatPath}", tempBatPath);
                }
                catch
                {
                    // Ignore cleanup errors
                }
            }

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
        // Use lib.exe for static libraries
        var libExe = FindLibExecutable();
        if (string.IsNullOrEmpty(libExe))
        {
            return new LinkResult
            {
                Success = false,
                StandardError = "lib.exe not found"
            };
        }

        var result = new LinkResult
        {
            OutputFile = options.OutputFile
        };

        var stopwatch = Stopwatch.StartNew();

        try
        {
            var arguments = BuildLibArguments(options);

            Log.Information("Creating static library {OutputFile} with lib.exe", options.OutputFile);
            Log.Debug("Arguments: {Arguments}", arguments);

            var startInfo = new ProcessStartInfo
            {
                FileName = libExe,
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
                result.StandardError = "Failed to start lib.exe process";
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
            "/nologo",
            "/machine:x64"
        };

        flags.AddRange(GetSubsystemFlags(targetType));

        // Incremental linking
        if (configuration == BuildConfiguration.Debug && targetType != LinkTargetType.StaticLibrary)
        {
            flags.Add("/incremental");
        }
        else
        {
            flags.Add("/incremental:no");
        }

        return flags;
    }

    public List<string> GetSubsystemFlags(LinkTargetType targetType)
    {
        return targetType switch
        {
            LinkTargetType.Executable => new List<string> { "/subsystem:console" },
            LinkTargetType.SharedLibrary => new List<string> { "/DLL" },
            LinkTargetType.StaticLibrary => new List<string>(),
            _ => new List<string>()
        };
    }

    private string BuildLinkArguments(LinkerOptions options)
    {
        var args = new List<string>();

        // Output
        args.Add($"/OUT:\"{options.OutputFile}\"");

        // Import library for DLLs
        if (options.TargetType == LinkTargetType.SharedLibrary && !string.IsNullOrEmpty(options.ImportLibrary))
        {
            args.Add($"/IMPLIB:\"{options.ImportLibrary}\"");
        }

        // Definition file
        if (!string.IsNullOrEmpty(options.DefinitionFile))
        {
            args.Add($"/DEF:\"{options.DefinitionFile}\"");
        }

        // Configuration-specific flags
        args.AddRange(GetDefaultFlags(options.Configuration, options.TargetType));

        // Debug info
        if (options.GenerateDebugInfo)
        {
            args.Add("/DEBUG");
            // Skip PDBALTPATH for now - it's causing linker errors
            // if (options.Configuration == BuildConfiguration.Debug)
            // {
            //     args.Add("/PDBALTPATH:%_PDB%");
            // }
        }

        // Library directories
        foreach (var libDir in options.LibraryDirectories)
        {
            args.Add($"/LIBPATH:\"{libDir}\"");
        }

        // Object files
        foreach (var objFile in options.ObjectFiles)
        {
            args.Add($"\"{objFile}\"");
        }

        // Libraries
        foreach (var lib in options.Libraries)
        {
            args.Add($"\"{lib}\"");
        }

        // Additional flags
        args.AddRange(options.AdditionalFlags);

        return string.Join(" ", args);
    }

    private string BuildLibArguments(LinkerOptions options)
    {
        var args = new List<string>();

        args.Add("/nologo");
        args.Add($"/OUT:\"{options.OutputFile}\"");

        // Object files
        foreach (var objFile in options.ObjectFiles)
        {
            args.Add($"\"{objFile}\"");
        }

        return string.Join(" ", args);
    }

    private string FindExecutable()
    {
        if (!string.IsNullOrEmpty(_executablePath))
        {
            return _executablePath;
        }

        // link.exe is in the same directory as cl.exe
        var clCompiler = new MSVCCompiler();
        var clPath = clCompiler.ExecutablePath;
        if (!string.IsNullOrEmpty(clPath) && File.Exists(clPath))
        {
            var linkPath = Path.Combine(Path.GetDirectoryName(clPath)!, "link.exe");
            if (File.Exists(linkPath))
            {
                _executablePath = linkPath;
                return linkPath;
            }
        }

        return string.Empty;
    }

    private string FindLibExecutable()
    {
        // lib.exe is in the same directory as cl.exe
        var clCompiler = new MSVCCompiler();
        var clPath = clCompiler.ExecutablePath;
        if (!string.IsNullOrEmpty(clPath) && File.Exists(clPath))
        {
            var libPath = Path.Combine(Path.GetDirectoryName(clPath)!, "lib.exe");
            if (File.Exists(libPath))
            {
                return libPath;
            }
        }

        return string.Empty;
    }

    private async Task<string?> GenerateExportDefinitionFileAsync(LinkerOptions options)
    {
        var dumpbinPath = FindDumpbinExecutable();
        if (string.IsNullOrEmpty(dumpbinPath))
        {
            Log.Warning("dumpbin.exe not found; cannot export all symbols for {Output}", options.OutputFile);
            return null;
        }

        var exports = new SortedSet<string>(StringComparer.Ordinal);
        foreach (var objectFile in options.ObjectFiles)
        {
            if (!File.Exists(objectFile))
            {
                continue;
            }

            var startInfo = new ProcessStartInfo
            {
                FileName = dumpbinPath,
                Arguments = $"/SYMBOLS \"{objectFile}\"",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var process = Process.Start(startInfo);
            if (process == null)
            {
                continue;
            }

            var output = await process.StandardOutput.ReadToEndAsync();
            await process.WaitForExitAsync();

            var normalizedOutput = System.Text.RegularExpressions.Regex.Replace(
                output,
                @"\r?\n(?=\?)",
                string.Empty);

            foreach (var line in normalizedOutput.Split('\n'))
            {
                if (!line.Contains("External", StringComparison.Ordinal) || line.Contains("UNDEF", StringComparison.Ordinal))
                {
                    continue;
                }

                var pipeIndex = line.LastIndexOf('|');
                if (pipeIndex < 0)
                {
                    continue;
                }

                var symbolPart = line[(pipeIndex + 1)..].Trim();
                var spaceIndex = symbolPart.IndexOf(' ');
                var symbol = spaceIndex >= 0 ? symbolPart[..spaceIndex] : symbolPart;
                if (string.IsNullOrWhiteSpace(symbol) || symbol.StartsWith("__imp_", StringComparison.Ordinal))
                {
                    continue;
                }

                if (ShouldExportSymbol(symbol))
                {
                    exports.Add(symbol);
                }
            }
        }

        if (exports.Count == 0)
        {
            return null;
        }

        var definitionFile = Path.Combine(
            options.WorkingDirectory,
            $"{Path.GetFileNameWithoutExtension(options.OutputFile)}_exports.def");

        await using var writer = new StreamWriter(definitionFile);
        await writer.WriteLineAsync("EXPORTS");
        foreach (var export in exports)
        {
            await writer.WriteLineAsync(export);
        }

        Log.Debug("Generated export definition file with {Count} symbols: {Path}", exports.Count, definitionFile);
        return definitionFile;
    }

    private static bool ShouldExportSymbol(string symbol)
    {
        if (symbol is "InitializeModule")
        {
            return true;
        }

        return symbol.Contains("@we@", StringComparison.Ordinal)
            || symbol.Contains("@WE@", StringComparison.Ordinal);
    }

    private string FindDumpbinExecutable()
    {
        if (!string.IsNullOrEmpty(_executablePath))
        {
            var dumpbinPath = Path.Combine(Path.GetDirectoryName(_executablePath)!, "dumpbin.exe");
            if (File.Exists(dumpbinPath))
            {
                return dumpbinPath;
            }
        }

        var clCompiler = new MSVCCompiler();
        var clPath = clCompiler.ExecutablePath;
        if (!string.IsNullOrEmpty(clPath) && File.Exists(clPath))
        {
            var dumpbinPath = Path.Combine(Path.GetDirectoryName(clPath)!, "dumpbin.exe");
            if (File.Exists(dumpbinPath))
            {
                return dumpbinPath;
            }
        }

        return string.Empty;
    }
}
