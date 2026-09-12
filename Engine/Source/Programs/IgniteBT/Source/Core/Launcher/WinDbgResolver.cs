// ==============================================================================
// WindEffects — IgniteBT — WinDbgResolver
// Source file for the IgniteBT module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.Win32;
using Serilog;
using IgniteBT.Build.Toolchain;

namespace IgniteBT.Core.Launcher;

public sealed class WinDbgLaunchOptions
{
    public string? ExecutablePath { get; set; }
    public string[] Arguments { get; set; } = Array.Empty<string>();
    public int? ProcessId { get; set; }
    public string? DumpPath { get; set; }
    public string WorkingDirectory { get; set; } = string.Empty;
    public bool PreferCdb { get; set; }
    public List<string> ExtraSymbolPaths { get; set; } = new();
}

/// <summary>
/// Discovers WinDbg/CDB installations and manages native debugging sessions.
/// </summary>
public static class WinDbgResolver
{
    private static readonly string[] SdkDebuggerRelativePaths = new[]
    {
        @"Debuggers\x64",
        @"Debuggers\x86",
        @"Debuggers\arm64"
    };

    public static string? FindDebuggerExecutable(bool preferCdb = false)
    {
        if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            Log.Warning("WinDbg native debugging is only supported on Windows.");
            return null;
        }

        var customPath = Environment.GetEnvironmentVariable("WE_WINDBG_PATH");
        if (!string.IsNullOrEmpty(customPath) && File.Exists(customPath))
        {
            return customPath;
        }

        var targetExe = preferCdb ? "cdb.exe" : "windbg.exe";

        // 1. Check Store / AppX WinDbg Preview executable in LocalAppData
        if (!preferCdb)
        {
            var localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            var winDbgXCandidate = Path.Combine(localAppData, @"Microsoft\WindowsApps\WinDbgX.exe");
            if (File.Exists(winDbgXCandidate))
            {
                return winDbgXCandidate;
            }
        }

        // 2. Check Windows SDK Program Files paths
        var programFilesX86 = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86);
        var programFiles = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);

        var kitBasePaths = new[]
        {
            Path.Combine(programFilesX86, "Windows Kits", "10"),
            Path.Combine(programFiles, "Windows Kits", "10")
        };

        foreach (var basePath in kitBasePaths)
        {
            foreach (var relPath in SdkDebuggerRelativePaths)
            {
                var candidate = Path.Combine(basePath, relPath, targetExe);
                if (File.Exists(candidate))
                {
                    return candidate;
                }
            }
        }

        // 3. Query Windows SDK Registry
        try
        {
            using var key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Windows Kits\Installed Roots");
            if (key != null)
            {
                var kitsRoot = key.GetValue("KitsRoot10")?.ToString();
                if (!string.IsNullOrEmpty(kitsRoot))
                {
                    foreach (var relPath in SdkDebuggerRelativePaths)
                    {
                        var candidate = Path.Combine(kitsRoot, relPath, targetExe);
                        if (File.Exists(candidate))
                        {
                            return candidate;
                        }
                    }
                }
            }
        }
        catch (Exception ex)
        {
            Log.Debug(ex, "Registry check for Windows SDK KitsRoot10 failed");
        }

        // 4. Fall back to system PATH resolution
        var pathEnv = Environment.GetEnvironmentVariable("PATH") ?? string.Empty;
        var searchNames = preferCdb
            ? new[] { "cdb.exe" }
            : new[] { "WinDbgX.exe", "windbg.exe", "cdb.exe" };

        foreach (var dir in pathEnv.Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries))
        {
            foreach (var name in searchNames)
            {
                var fullPath = Path.Combine(dir.Trim(), name);
                if (File.Exists(fullPath))
                {
                    return fullPath;
                }
            }
        }

        return null;
    }

    public static string BuildSymbolPath(string outputRoot, IEnumerable<string>? extraPaths = null)
    {
        var symbolPaths = new List<string>();

        if (!string.IsNullOrWhiteSpace(outputRoot) && Directory.Exists(outputRoot))
        {
            symbolPaths.Add(outputRoot);

            var thirdPartyPath = Path.Combine(outputRoot, "ThirdParty");
            if (Directory.Exists(thirdPartyPath))
            {
                symbolPaths.Add(thirdPartyPath);
            }
        }

        if (extraPaths != null)
        {
            foreach (var path in extraPaths)
            {
                if (!string.IsNullOrWhiteSpace(path) && Directory.Exists(path) && !symbolPaths.Contains(path))
                {
                    symbolPaths.Add(path);
                }
            }
        }

        // Standard Microsoft public symbol server cache setup
        var tempCache = Path.Combine(Path.GetTempPath(), "SymbolCache");
        Directory.CreateDirectory(tempCache);
        symbolPaths.Add($"srv*{tempCache}*https:

        return string.Join(";", symbolPaths);
    }

    public static int LaunchDebugger(WinDbgLaunchOptions options)
    {
        var debuggerExe = FindDebuggerExecutable(options.PreferCdb);
        if (string.IsNullOrEmpty(debuggerExe) || !File.Exists(debuggerExe))
        {
            Log.Error("WinDbg/CDB executable not found. Please install Windows SDK Debuggers or WinDbg Preview.");
            Log.Error("Alternatively, set the WE_WINDBG_PATH environment variable to windbg.exe / WinDbgX.exe.");
            return 1;
        }

        Log.Information("Using native debugger: {DebuggerPath}", debuggerExe);

        var symbolPath = BuildSymbolPath(options.WorkingDirectory, options.ExtraSymbolPaths);
        Log.Information("Symbol Path: {SymbolPath}", symbolPath);

        var workDir = options.WorkingDirectory;
        if (string.IsNullOrEmpty(workDir) || !Directory.Exists(workDir))
        {
            workDir = Environment.CurrentDirectory;
        }

        var startInfo = new ProcessStartInfo
        {
            FileName = debuggerExe,
            WorkingDirectory = workDir,
            UseShellExecute = false
        };

        startInfo.Environment["_NT_SYMBOL_PATH"] = symbolPath;

        var pathEntries = new List<string>();
        if (!string.IsNullOrEmpty(options.WorkingDirectory) && Directory.Exists(options.WorkingDirectory))
        {
            pathEntries.Add(options.WorkingDirectory);

            var engineBinaries = Path.Combine(options.WorkingDirectory, "Engine", "Binaries");
            if (Directory.Exists(engineBinaries))
            {
                pathEntries.Add(engineBinaries);
            }

            var thirdParty = Path.Combine(options.WorkingDirectory, "ThirdParty");
            if (Directory.Exists(thirdParty))
            {
                pathEntries.Add(thirdParty);
            }
        }

        try
        {
            var compiler = ToolchainDetector.DetectCompiler();
            if (compiler.Type == CompilerType.MSVC && !string.IsNullOrEmpty(compiler.VcVarsAllPath))
            {
                var vcEnv = VcEnvironmentCache.LoadOrCapture(compiler.VcVarsAllPath);
                if (vcEnv != null && vcEnv.TryGetValue("PATH", out var vcPath))
                {
                    pathEntries.Add(vcPath);
                }
            }

            EnsureDebugCrtDlls(workDir, compiler.VsInstallPath, pathEntries);
        }
        catch (Exception ex)
        {
            Log.Debug(ex, "Failed to query MSVC environment for Debug CRT paths");
        }

        var existingPath = Environment.GetEnvironmentVariable("PATH") ?? string.Empty;
        if (!string.IsNullOrEmpty(existingPath))
        {
            pathEntries.Add(existingPath);
        }

        startInfo.Environment["PATH"] = string.Join(Path.PathSeparator.ToString(), pathEntries.Where(p =>
            !string.IsNullOrWhiteSpace(p)).Distinct());

        var isWinDbgX = Path.GetFileName(debuggerExe).Equals("WinDbgX.exe", StringComparison.OrdinalIgnoreCase);

        // WinDbg flags:
        // -y <symbolpath>
        // -z <dumpfile>
        // -p <pid>
        if (!isWinDbgX)
        {
            startInfo.ArgumentList.Add("-g");
            startInfo.ArgumentList.Add("-y");
            startInfo.ArgumentList.Add(symbolPath);
        }

        if (!string.IsNullOrEmpty(options.DumpPath))
        {
            Log.Information("Opening crash dump in WinDbg: {DumpPath}", options.DumpPath);
            startInfo.ArgumentList.Add("-z");
            startInfo.ArgumentList.Add(options.DumpPath);
        }
        else if (options.ProcessId.HasValue)
        {
            Log.Information("Attaching WinDbg to process PID: {PID}", options.ProcessId.Value);
            startInfo.ArgumentList.Add("-p");
            startInfo.ArgumentList.Add(options.ProcessId.Value.ToString());
        }
        else if (!string.IsNullOrEmpty(options.ExecutablePath))
        {
            Log.Information("Launching target binary under WinDbg: {Executable}", options.ExecutablePath);
            startInfo.ArgumentList.Add(options.ExecutablePath);

            foreach (var arg in options.Arguments)
            {
                startInfo.ArgumentList.Add(arg);
            }
        }
        else
        {
            Log.Error("No target binary, PID, or dump file specified for WinDbg session.");
            return 1;
        }

        try
        {
            var process = Process.Start(startInfo);
            if (process != null)
            {
                Log.Information("Native debugging session started (Process ID: {PID})", process.Id);
                return 0;
            }
            Log.Error("Failed to start WinDbg process.");
            return 1;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Failed to launch WinDbg debugger process");
            return 1;
        }
    }

    public static void EnsureDebugCrtDlls(string outputDirectory, string? vsInstallPath, List<string>? outPathEntries =
        null)
    {
        if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            return;
        }

        var crtDllNames = new[]
        {
            "msvcp140d.dll",
            "msvcp140d_atomic_wait.dll",
            "msvcp140d_codecvt_ids.dll",
            "msvcp140_1d.dll",
            "msvcp140_2d.dll",
            "vcruntime140d.dll",
            "vcruntime140_1d.dll",
            "vcruntime140_threadsd.dll",
            "concrt140d.dll",
            "vccorlib140d.dll"
        };

        // 1. Locate MSVC Debug CRT
        var searchRoots = new List<string>();
        if (!string.IsNullOrEmpty(vsInstallPath) && Directory.Exists(vsInstallPath))
        {
            searchRoots.Add(Path.Combine(vsInstallPath, "VC", "Redist", "MSVC"));
        }

        var programFilesX86 = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86);
        var programFiles = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);
        searchRoots.Add(Path.Combine(programFilesX86, "Microsoft Visual Studio"));
        searchRoots.Add(Path.Combine(programFiles, "Microsoft Visual Studio"));

        // Drive roots (e.g. F:\vs)
        foreach (var drive in Directory.GetLogicalDrives())
        {
            var candidateVs = Path.Combine(drive, "vs", "VC", "Redist", "MSVC");
            if (Directory.Exists(candidateVs) && !searchRoots.Contains(candidateVs))
            {
                searchRoots.Add(candidateVs);
            }
        }

        foreach (var root in searchRoots)
        {
            if (!Directory.Exists(root)) continue;

            try
            {
                var debugDirs = Directory.GetDirectories(root, "*DebugCRT*", SearchOption.AllDirectories);
                foreach (var dir in debugDirs)
                {
                    if (dir.EndsWith(@"\x64", StringComparison.OrdinalIgnoreCase) || dir.Contains(@"\x64\", StringComparison.OrdinalIgnoreCase))
                    {
                        if (outPathEntries != null && !outPathEntries.Contains(dir))
                        {
                            outPathEntries.Add(dir);
                        }

                        CopyDllsIfMissing(dir, outputDirectory, crtDllNames);
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Debug(ex, "Error scanning MSVC Debug CRT directory {Root}", root);
            }
        }

        // 2. Locate Windows Kits UCRT Debug DLL (ucrtbased.dll)
        var kitRoots = new[]
        {
            Path.Combine(programFilesX86, "Windows Kits", "10", "bin"),
            Path.Combine(programFiles, "Windows Kits", "10", "bin")
        };

        foreach (var kitRoot in kitRoots)
        {
            if (!Directory.Exists(kitRoot)) continue;

            try
            {
                var versionDirs = Directory.GetDirectories(kitRoot);
                foreach (var versionDir in versionDirs)
                {
                    var ucrtCandidates = new[]
                    {
                        Path.Combine(versionDir, "x64", "ucrt"),
                        Path.Combine(versionDir, "x64")
                    };

                    foreach (var ucrtDir in ucrtCandidates)
                    {
                        if (Directory.Exists(ucrtDir) && File.Exists(Path.Combine(ucrtDir, "ucrtbased.dll")))
                        {
                            if (outPathEntries != null && !outPathEntries.Contains(ucrtDir))
                            {
                                outPathEntries.Add(ucrtDir);
                            }

                            CopyDllsIfMissing(ucrtDir, outputDirectory, new[] { "ucrtbased.dll" });
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Debug(ex, "Error scanning Windows Kits UCRT directory {KitRoot}", kitRoot);
            }
        }
    }

    private static void CopyDllsIfMissing(string sourceDir, string targetDir, string[] dllNames)
    {
        if (!Directory.Exists(targetDir)) return;

        foreach (var dll in dllNames)
        {
            var sourcePath = Path.Combine(sourceDir, dll);
            if (File.Exists(sourcePath))
            {
                var targetPath = Path.Combine(targetDir, dll);
                if (!File.Exists(targetPath))
                {
                    try
                    {
                        File.Copy(sourcePath, targetPath, overwrite: false);
                        Log.Information("Copied Debug CRT DLL to output directory: {DllName}", dll);
                    }
                    catch (Exception ex)
                    {
                        Log.Debug(ex, "Could not copy {Dll} to {TargetDir}", dll, targetDir);
                    }
                }
            }
        }
    }
}

