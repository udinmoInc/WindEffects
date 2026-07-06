namespace IgniteBT.Core.Launcher;

/// <summary>
/// Redirects toolchain temp, NuGet, and SDK cache paths to the project drive (F:)
/// instead of %LocalAppData% on C: when disk space is tight.
/// </summary>
public static class BuildEnvironment
{
    public static string? ProjectRoot { get; private set; }

    /// <summary>
    /// Local app-data directory for tooling writes. Prefers redirected LOCALAPPDATA
    /// (project .toolchain/localappdata on F:) over the Windows profile path on C:.
    /// </summary>
    public static string ResolveLocalAppData()
    {
        var fromEnv = Environment.GetEnvironmentVariable("LOCALAPPDATA");
        if (!string.IsNullOrWhiteSpace(fromEnv))
            return Path.GetFullPath(fromEnv);

        if (!string.IsNullOrWhiteSpace(ProjectRoot))
            return Path.Combine(ProjectRoot, ".toolchain", "localappdata");

        return Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
    }

    /// <summary>
    /// Temp directory for compiler/tooling scratch. Prefers redirected TEMP/TMP on the project drive.
    /// </summary>
    public static string ResolveTemp()
    {
        var fromEnv = Environment.GetEnvironmentVariable("TEMP")
            ?? Environment.GetEnvironmentVariable("TMP");
        if (!string.IsNullOrWhiteSpace(fromEnv))
            return Path.GetFullPath(fromEnv);

        if (!string.IsNullOrWhiteSpace(ProjectRoot))
            return Path.Combine(ProjectRoot, "Build", "Temp");

        return Path.GetTempPath();
    }

    /// <summary>
    /// Project-local database directory (SDK cache, build metadata) on the repo drive.
    /// </summary>
    public static string ResolveProjectDatabaseDirectory(string? projectRoot = null)
    {
        projectRoot ??= ProjectRoot ?? ResolveProjectRoot();
        if (!string.IsNullOrWhiteSpace(projectRoot))
            return Path.Combine(projectRoot, "Build", "Database");

        return Path.Combine(Directory.GetCurrentDirectory(), "Build", "Database");
    }

    /// <summary>
    /// Applies F:-drive-friendly paths for the given project root (or current directory).
    /// Safe to call multiple times; only creates directories once per process.
    /// </summary>
    public static void Configure(string? projectRoot = null)
    {
        projectRoot ??= ResolveProjectRoot();
        if (string.IsNullOrWhiteSpace(projectRoot))
            return;

        ProjectRoot = projectRoot;

        var toolchainDir = Path.Combine(projectRoot, ".toolchain");
        var buildTempDir = Path.Combine(projectRoot, "Build", "Temp");
        var localAppDataDir = Path.Combine(toolchainDir, "localappdata");
        var nugetPackages = Path.Combine(projectRoot, ".nuget", "packages");

        SetEnv("WE_PROJECT_ROOT", projectRoot);
        SetEnv("TEMP", buildTempDir);
        SetEnv("TMP", buildTempDir);
        SetEnv("NUGET_PACKAGES", nugetPackages);
        SetEnv("NUGET_HTTP_CACHE_PATH", Path.Combine(toolchainDir, "nuget-http-cache"));
        SetEnv("DOTNET_CLI_HOME", Path.Combine(toolchainDir, "dotnet-cli"));
        SetEnv("DOTNET_SKIP_FIRST_TIME_EXPERIENCE", "1");
        SetEnv("DOTNET_NOLOGO", "1");

        // Keep SDK/tooling writes off C: (e.g. IgniteBT.SDKCache.json, dotnet first-run).
        if (OperatingSystem.IsWindows())
            SetEnv("LOCALAPPDATA", localAppDataDir);

        foreach (var dir in new[]
                 {
                     buildTempDir, toolchainDir, localAppDataDir, nugetPackages,
                     Path.Combine(toolchainDir, "nuget-http-cache"),
                     Path.Combine(toolchainDir, "dotnet-cli"),
                     ResolveProjectDatabaseDirectory(projectRoot)
                 })
        {
            Directory.CreateDirectory(dir);
        }
    }

    /// <summary>
    /// Copies F:-drive path overrides into a child process environment (e.g. build daemon).
    /// </summary>
    public static void ApplyToProcessEnvironment(System.Diagnostics.ProcessStartInfo startInfo, string? projectRoot = null)
    {
        Configure(projectRoot);
        if (string.IsNullOrWhiteSpace(ProjectRoot))
            return;

        var toolchainDir = Path.Combine(ProjectRoot, ".toolchain");
        var buildTempDir = Path.Combine(ProjectRoot, "Build", "Temp");
        var localAppDataDir = Path.Combine(toolchainDir, "localappdata");
        var nugetPackages = Path.Combine(ProjectRoot, ".nuget", "packages");

        startInfo.Environment["WE_PROJECT_ROOT"] = ProjectRoot;
        startInfo.Environment["TEMP"] = buildTempDir;
        startInfo.Environment["TMP"] = buildTempDir;
        startInfo.Environment["NUGET_PACKAGES"] = nugetPackages;
        startInfo.Environment["NUGET_HTTP_CACHE_PATH"] = Path.Combine(toolchainDir, "nuget-http-cache");
        startInfo.Environment["DOTNET_CLI_HOME"] = Path.Combine(toolchainDir, "dotnet-cli");
        startInfo.Environment["DOTNET_SKIP_FIRST_TIME_EXPERIENCE"] = "1";
        startInfo.Environment["DOTNET_NOLOGO"] = "1";
        if (OperatingSystem.IsWindows())
            startInfo.Environment["LOCALAPPDATA"] = localAppDataDir;
    }

    private static string? ResolveProjectRoot()
    {
        var envRoot = Environment.GetEnvironmentVariable("WE_PROJECT_ROOT")
            ?? Environment.GetEnvironmentVariable("WE_ENGINE_ROOT");
        if (!string.IsNullOrWhiteSpace(envRoot))
        {
            var candidate = Path.GetFullPath(envRoot);
            if (Directory.Exists(Path.Combine(candidate, "Engine", "Source"))
                || File.Exists(Path.Combine(candidate, EngineDescriptor.FileName)))
                return candidate;
        }

        if (EngineDescriptor.TryFindEngineRoot(Directory.GetCurrentDirectory(), out var engineRoot))
            return engineRoot;

        return IgniteBT.Build.Layout.BuildLayout.FindProjectRoot(Directory.GetCurrentDirectory());
    }

    private static void SetEnv(string name, string value)
    {
        var current = Environment.GetEnvironmentVariable(name);
        if (!string.Equals(current, value, StringComparison.OrdinalIgnoreCase))
            Environment.SetEnvironmentVariable(name, value);
    }
}
