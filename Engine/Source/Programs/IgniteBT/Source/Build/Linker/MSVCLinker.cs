using System.Diagnostics;
using Serilog;
using IgniteBT.Build.Compiler;
using IgniteBT.Build.Toolchain;
using IgniteBT.Core.Cache;
using IgniteBT.Core.Hashing;

namespace IgniteBT.Build.Linker;

/// <summary>
/// Microsoft Visual C++ linker implementation.
/// </summary>
public class MSVCLinker : ILinker
{
    private string? _version;
    private string? _executablePath;
    private Dictionary<string, string>? _environmentVariables;
    private string? _vcVarsAllPath;
    private ExportDefinitionCache? _exportCache;
    private FileHashCache? _fileHashes;

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

    public void SetExportDefinitionCache(ExportDefinitionCache cache, FileHashCache fileHashes)
    {
        _exportCache = cache;
        _fileHashes = fileHashes;
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
                WorkingDirectory = options.WorkingDirectory
            };
            VcEnvironmentCache.ApplyToProcessStartInfo(startInfo, cachedEnv);

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

            var cachedEnv = _environmentVariables
                ?? VcEnvironmentCache.LoadOrCapture(_vcVarsAllPath);

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
            VcEnvironmentCache.ApplyToProcessStartInfo(startInfo, cachedEnv);

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

    public List<string> GetDefaultFlags(BuildConfiguration configuration, LinkTargetType targetType) =>
        GetDefaultFlags(configuration, targetType, subsystem: null);

    public List<string> GetDefaultFlags(BuildConfiguration configuration, LinkTargetType targetType, string? subsystem)
    {
        var flags = new List<string>
        {
            "/nologo",
            "/machine:x64"
        };

        flags.AddRange(GetSubsystemFlags(targetType, subsystem));

        // Incremental linking for fast iteration (Debug + Development)
        if (configuration is BuildConfiguration.Debug or BuildConfiguration.Development
            && targetType != LinkTargetType.StaticLibrary)
        {
            flags.Add("/incremental");
        }
        else
        {
            flags.Add("/incremental:no");
        }

        // LTCG only for Shipping (requires /GL object files from compiler)
        if (configuration == BuildConfiguration.Shipping && targetType != LinkTargetType.StaticLibrary)
        {
            flags.Add("/LTCG");
        }

        return flags;
    }

    public List<string> GetSubsystemFlags(LinkTargetType targetType) =>
        GetSubsystemFlags(targetType, subsystem: null);

    public List<string> GetSubsystemFlags(LinkTargetType targetType, string? subsystem)
    {
        if (!string.IsNullOrWhiteSpace(subsystem))
        {
            return new List<string> { $"/subsystem:{subsystem.ToLowerInvariant()}" };
        }

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

        // Import library for DLLs and executables that export symbols
        if (!string.IsNullOrEmpty(options.ImportLibrary)
            && options.TargetType is LinkTargetType.SharedLibrary or LinkTargetType.Executable)
        {
            args.Add($"/IMPLIB:\"{options.ImportLibrary}\"");
        }

        // Definition file
        if (!string.IsNullOrEmpty(options.DefinitionFile))
        {
            args.Add($"/DEF:\"{options.DefinitionFile}\"");
        }

        if (!string.IsNullOrEmpty(options.ProgramDatabaseFile))
        {
            args.Add($"/PDB:\"{options.ProgramDatabaseFile}\"");
        }

        if (!string.IsNullOrEmpty(options.IncrementalLinkDatabaseFile))
        {
            args.Add($"/ILK:\"{options.IncrementalLinkDatabaseFile}\"");
        }

        // Configuration-specific flags
        args.AddRange(GetDefaultFlags(options.Configuration, options.TargetType, options.Subsystem));

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
        var perObjectDefs = new List<string>();
        var allCached = _exportCache != null && _fileHashes != null;

        foreach (var objectFile in options.ObjectFiles)
        {
            if (!File.Exists(objectFile)) continue;

            if (allCached && _exportCache!.TryGetDefinitionPath(objectFile, _fileHashes!, out var cachedDef))
            {
                perObjectDefs.Add(cachedDef!);
                continue;
            }

            var objectExports = await ScanObjectExportsAsync(objectFile);
            if (objectExports.Count == 0) continue;

            var defContent = "EXPORTS\n" + string.Join('\n', objectExports);
            if (allCached)
            {
                var cachedPath = _exportCache!.StoreDefinition(objectFile, _fileHashes!, defContent);
                perObjectDefs.Add(cachedPath);
            }
            else
            {
                var tempDef = Path.Combine(options.WorkingDirectory,
                    $"{Path.GetFileNameWithoutExtension(objectFile)}_exports.def");
                await File.WriteAllTextAsync(tempDef, defContent);
                perObjectDefs.Add(tempDef);
            }
        }

        if (perObjectDefs.Count == 0) return null;

        if (_exportCache != null)
        {
            var moduleName = Path.GetFileNameWithoutExtension(options.OutputFile);
            return _exportCache.GetCombinedDefinitionPath(moduleName, options.WorkingDirectory, perObjectDefs);
        }

        var exports = new SortedSet<string>(StringComparer.Ordinal);
        foreach (var defPath in perObjectDefs)
        {
            var inExports = false;
            foreach (var line in await File.ReadAllLinesAsync(defPath))
            {
                if (line.Equals("EXPORTS", StringComparison.OrdinalIgnoreCase)) { inExports = true; continue; }
                if (inExports && !string.IsNullOrWhiteSpace(line)) exports.Add(line.Trim());
            }
        }

        var definitionFile = Path.Combine(
            options.WorkingDirectory,
            $"{Path.GetFileNameWithoutExtension(options.OutputFile)}_exports.def");
        await using var writer = new StreamWriter(definitionFile);
        await writer.WriteLineAsync("EXPORTS");
        foreach (var export in exports)
            await writer.WriteLineAsync(export);

        Log.Debug("Generated export definition file with {Count} symbols: {Path}", exports.Count, definitionFile);
        return definitionFile;
    }

    private async Task<SortedSet<string>> ScanObjectExportsAsync(string objectFile)
    {
        var exports = new SortedSet<string>(StringComparer.Ordinal);
        var dumpbinPath = FindDumpbinExecutable();
        if (string.IsNullOrEmpty(dumpbinPath)) return exports;

        var cachedEnv = _environmentVariables ?? VcEnvironmentCache.LoadOrCapture(_vcVarsAllPath);
        var startInfo = new ProcessStartInfo
        {
            FileName = dumpbinPath,
            Arguments = $"/SYMBOLS \"{objectFile}\"",
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true
        };
        VcEnvironmentCache.ApplyToProcessStartInfo(startInfo, cachedEnv);

        using var process = Process.Start(startInfo);
        if (process == null) return exports;

        var output = await process.StandardOutput.ReadToEndAsync();
        await process.WaitForExitAsync();

        var normalizedOutput = System.Text.RegularExpressions.Regex.Replace(output, @"\r?\n(?=\?)", string.Empty);
        foreach (var line in normalizedOutput.Split('\n'))
        {
            if (!line.Contains("External", StringComparison.Ordinal) || line.Contains("UNDEF", StringComparison.Ordinal))
                continue;

            var pipeIndex = line.LastIndexOf('|');
            if (pipeIndex < 0) continue;

            var symbolPart = line[(pipeIndex + 1)..].Trim();
            var spaceIndex = symbolPart.IndexOf(' ');
            var symbol = spaceIndex >= 0 ? symbolPart[..spaceIndex] : symbolPart;
            if (string.IsNullOrWhiteSpace(symbol) || symbol.StartsWith("__imp_", StringComparison.Ordinal))
                continue;

            if (ShouldExportSymbol(symbol))
                exports.Add(symbol);
        }

        return exports;
    }

    private static bool ShouldExportSymbol(string symbol)
    {
        if (symbol is "InitializeModule")
        {
            return true;
        }

        if (symbol.StartsWith("$", StringComparison.Ordinal))
        {
            return false;
        }

        // Scalar/vector deleting destructors must never be exported (LNK4102).
        // Exporting them makes delete/new and cross-module teardown call the wrong free.
        if (symbol.StartsWith("??_G", StringComparison.Ordinal) ||
            symbol.StartsWith("??_E", StringComparison.Ordinal))
        {
            return false;
        }

        // STL symbols pulled in via templates/members — not part of the module ABI.
        if (symbol.Contains("@std@@", StringComparison.Ordinal))
        {
            return false;
        }

        // Internal methods that must not be exported (avoids EH metadata link failures).
        if (symbol.Contains("SetExternalSearchFilter", StringComparison.Ordinal))
        {
            return false;
        }

        if (symbol.Contains("ModuleManager", StringComparison.Ordinal))
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
