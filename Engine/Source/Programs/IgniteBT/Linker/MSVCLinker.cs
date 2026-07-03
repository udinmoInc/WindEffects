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

    public string Name => "MSVC LINK";
    public string ExecutablePath => _executablePath ?? FindExecutable();
    public string Version => _version ?? string.Empty;
    public TargetPlatform Platform => TargetPlatform.Windows;

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
            var arguments = BuildLinkArguments(options);

            Log.Information("Linking {TargetType} to {OutputFile} with MSVC LINK", options.TargetType, options.OutputFile);
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
            if (options.Configuration == BuildConfiguration.Debug)
            {
                args.Add("/PDBALTPATH:%_PDB%");
            }
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
}
