using System.Diagnostics;
using Serilog;

namespace IgniteBT.Build.Compiler;

///
/// Windows resource (.rc) compilation via rc.exe. Discovers per-module
/// Resources/Resource.rc-style files, compiles them to .res next to the
/// module objects, and returns paths the linker consumes transparently
/// (MSVC LINK accepts .res inputs alongside .obj files).
///
public static class WindowsResourceCompiler
{
    public static bool IsSupported(string platform)
    {
        return platform.StartsWith("Win", StringComparison.OrdinalIgnoreCase);
    }

    public static List<string> DiscoverResourceFiles(string moduleDirectory)
    {
        if (!Directory.Exists(moduleDirectory)) return new();
        return Directory.GetFiles(moduleDirectory, "*.rc", SearchOption.AllDirectories)
            .OrderBy(f => f, StringComparer.OrdinalIgnoreCase)
            .ToList();
    }

    /// <summary>
    /// Files the .rc references (ICON/CURSOR/BITMAP art, #include headers),
    /// resolved against the .rc directory. Used for freshness + build hashing.
    /// </summary>
    public static List<string> GetResourceDependencies(string rcFile)
    {
        var deps = new List<string>();
        string? rcDir;
        try
        {
            rcDir = Path.GetDirectoryName(Path.GetFullPath(rcFile));
        }
        catch
        {
            return deps;
        }
        if (string.IsNullOrEmpty(rcDir)) return deps;

        string[] lines;
        try
        {
            lines = File.ReadAllLines(rcFile);
        }
        catch
        {
            return deps;
        }

        foreach (var rawLine in lines)
        {
            var line = rawLine.Trim();
            if (line.Length == 0 || line.StartsWith("//")) continue;

            string? quoted = null;
            if (line.StartsWith("#include", StringComparison.Ordinal))
            {
                quoted = ExtractQuotedPath(line.Substring("#include".Length));
            }
            else
            {
                // Resource statements: <id> <TYPE> "file" (ICON, CURSOR,
                // BITMAP, RCDATA, MANIFEST, HTML, ...).
                var firstQuote = line.IndexOf('"');
                var lastQuote = line.LastIndexOf('"');
                if (firstQuote >= 0 && lastQuote > firstQuote)
                {
                    var head = line[..firstQuote].Trim().Split(
                        (char[]?)null, StringSplitOptions.RemoveEmptyEntries);
                    if (head.Length >= 2)
                    {
                        quoted = line.Substring(firstQuote + 1, lastQuote - firstQuote - 1);
                    }
                }
            }

            if (string.IsNullOrWhiteSpace(quoted)) continue;
            string full;
            try
            {
                full = Path.GetFullPath(Path.Combine(rcDir, quoted.Trim()));
            }
            catch
            {
                continue;
            }
            if (File.Exists(full) && !deps.Contains(full, StringComparer.OrdinalIgnoreCase))
            {
                deps.Add(full);
            }
        }

        return deps;
    }

    public static bool IsUpToDate(string rcFile, string resFile)
    {
        if (!File.Exists(resFile)) return false;
        DateTime resTime;
        try
        {
            resTime = File.GetLastWriteTimeUtc(resFile);
        }
        catch
        {
            return false;
        }
        var inputs = new List<string> { rcFile };
        inputs.AddRange(GetResourceDependencies(rcFile));
        foreach (var input in inputs)
        {
            try
            {
                if (!File.Exists(input) || File.GetLastWriteTimeUtc(input) > resTime)
                {
                    return false;
                }
            }
            catch
            {
                return false;
            }
        }
        return true;
    }

    public static string? FindRcExe()
    {
        // Alongside cl.exe (some toolchains stage it there).
        var cl = new MSVCCompiler();
        var clPath = cl.ExecutablePath;
        if (!string.IsNullOrEmpty(clPath))
        {
            var dir = Path.GetDirectoryName(clPath);
            if (!string.IsNullOrEmpty(dir))
            {
                var candidate = Path.Combine(dir, "rc.exe");
                if (File.Exists(candidate)) return candidate;
            }
        }

        // Windows SDK versioned bin dirs — prefer the newest kit.
        const string kitsBin = @"C:\Program Files (x86)\Windows Kits\10\bin";
        if (Directory.Exists(kitsBin))
        {
            var best = Directory.GetDirectories(kitsBin)
                .Select(d => new { Dir = d, Rc = Path.Combine(d, "x64", "rc.exe") })
                .Where(x => File.Exists(x.Rc))
                .OrderByDescending(x => x.Dir, StringComparer.OrdinalIgnoreCase)
                .FirstOrDefault();
            if (best != null) return best.Rc;
        }

        // PATH fallback (vcvarsall / developer prompt environments).
        var pathEnv = Environment.GetEnvironmentVariable("PATH") ?? string.Empty;
        foreach (var dir in pathEnv.Split(';', StringSplitOptions.RemoveEmptyEntries))
        {
            try
            {
                var candidate = Path.Combine(dir.Trim(), "rc.exe");
                if (File.Exists(candidate)) return candidate;
            }
            catch
            {
                // Malformed PATH entries must not break discovery.
            }
        }

        return null;
    }

    public static async Task<string> CompileAsync(
        string rcExe, string rcFile, string resFile, IEnumerable<string> includeDirs)
    {
        var args = new List<string> { "/nologo", $"/fo\"{resFile}\"" };
        foreach (var dir in includeDirs.Distinct(StringComparer.OrdinalIgnoreCase))
        {
            if (!string.IsNullOrWhiteSpace(dir) && Directory.Exists(dir))
            {
                args.Add($"/i\"{dir}\"");
            }
        }
        args.Add($"\"{rcFile}\"");

        var resDir = Path.GetDirectoryName(resFile);
        if (!string.IsNullOrEmpty(resDir)) Directory.CreateDirectory(resDir);

        Log.Information("Compiling resources {Rc} with rc.exe", rcFile);
        var startInfo = new ProcessStartInfo
        {
            FileName = rcExe,
            Arguments = string.Join(" ", args),
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
            WorkingDirectory = Path.GetDirectoryName(rcFile) ?? resDir ?? string.Empty
        };

        using var process = Process.Start(startInfo);
        if (process == null)
        {
            throw new InvalidOperationException($"Failed to start rc.exe for {rcFile}");
        }

        var stdout = await process.StandardOutput.ReadToEndAsync();
        var stderr = await process.StandardError.ReadToEndAsync();
        await process.WaitForExitAsync();

        if (process.ExitCode != 0 || !File.Exists(resFile))
        {
            throw new InvalidOperationException(
                $"rc.exe failed for {rcFile} (exit {process.ExitCode}): {stderr} {stdout}".Trim());
        }

        Log.Information("Compiled resources {Rc} in {Res}", rcFile, resFile);
        return resFile;
    }

    private static string? ExtractQuotedPath(string text)
    {
        var first = text.IndexOf('"');
        var last = text.LastIndexOf('"');
        if (first < 0 || last <= first) return null;
        return text.Substring(first + 1, last - first - 1);
    }
}
