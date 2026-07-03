using System.Text.Json;
using IgniteBT.BuildSystem;

namespace IgniteBT.Launcher;

/// <summary>
/// Resolves engine/project roots and IgniteBT executable locations for CLI launchers.
/// </summary>
public static class EngineInstallation
{
    private const string EngineRootEnvVar = "WE_ENGINE_ROOT";
    private const string ProjectRootEnvVar = "WE_PROJECT_ROOT";
    private const string InstallFolderName = "WindEffects";
    private const string ManifestFileName = "engine.json";

    public sealed class EngineLocation
    {
        public required string ProjectRoot { get; init; }
        public required string EngineRoot { get; init; }
        public required string LauncherRoot { get; init; }
        public required string IgniteBtProjectPath { get; init; }
    }

    public static EngineLocation Resolve(string? startDirectory = null)
    {
        var start = startDirectory ?? Directory.GetCurrentDirectory();

        if (TryResolveFromEnvironment(out var fromEnv))
        {
            return fromEnv;
        }

        if (TryResolveFromInstallManifest(out var fromManifest))
        {
            return fromManifest;
        }

        if (TryResolveFromLauncherLocation(start, out var fromLauncher))
        {
            return fromLauncher;
        }

        var projectRoot = BuildLayout.FindProjectRoot(start)
            ?? throw new InvalidOperationException(
                "Could not locate the WindEffects engine. Set WE_PROJECT_ROOT, run `we setup`, or invoke `we` from the repository root.");

        return CreateLocation(projectRoot);
    }

    public static string ResolveIgniteBtExecutable(EngineLocation location, string configuration = "Debug")
    {
        var projectRoot = location.ProjectRoot;
        var candidates = new[]
        {
            Path.Combine(projectRoot, "Build", "Intermediate", "IgniteBT", configuration, "net8.0", GetIgniteBtBinaryName()),
            Path.Combine(projectRoot, "Build", "Intermediate", "IgniteBT", "Release", "net8.0", GetIgniteBtBinaryName()),
            Path.Combine(projectRoot, "Build", "Intermediate", "IgniteBT", "Debug", "net8.0", GetIgniteBtBinaryName()),
        };

        foreach (var candidate in candidates.Distinct(StringComparer.OrdinalIgnoreCase))
        {
            if (File.Exists(candidate))
            {
                return candidate;
            }
        }

        return string.Empty;
    }

    public static string GetIgniteBtBinaryName() =>
        OperatingSystem.IsWindows() ? "IgniteBT.exe" : "IgniteBT";

    public static string GetInstallManifestPath()
    {
        if (OperatingSystem.IsWindows())
        {
            var localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            return Path.Combine(localAppData, InstallFolderName, ManifestFileName);
        }

        var configHome = Environment.GetEnvironmentVariable("XDG_CONFIG_HOME");
        if (string.IsNullOrWhiteSpace(configHome))
        {
            configHome = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
                ".config");
        }

        return Path.Combine(configHome, "windeffects", ManifestFileName);
    }

    public static string GetInstalledLauncherDirectory()
    {
        if (OperatingSystem.IsWindows())
        {
            var localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            return Path.Combine(localAppData, InstallFolderName, "bin");
        }

        var home = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        return Path.Combine(home, ".local", "bin");
    }

    public static void WriteInstallManifest(EngineLocation location)
    {
        var manifestPath = GetInstallManifestPath();
        Directory.CreateDirectory(Path.GetDirectoryName(manifestPath)!);

        var manifest = new InstallManifest
        {
            ProjectRoot = location.ProjectRoot,
            EngineRoot = location.EngineRoot,
            LauncherRoot = location.LauncherRoot,
            InstalledAtUtc = DateTime.UtcNow
        };

        File.WriteAllText(manifestPath, JsonSerializer.Serialize(manifest, new JsonSerializerOptions
        {
            WriteIndented = true
        }));
    }

    public static bool TryReadInstallManifest(out InstallManifest manifest)
    {
        manifest = new InstallManifest();
        var manifestPath = GetInstallManifestPath();
        if (!File.Exists(manifestPath))
        {
            return false;
        }

        try
        {
            manifest = JsonSerializer.Deserialize<InstallManifest>(File.ReadAllText(manifestPath))
                ?? new InstallManifest();
            return !string.IsNullOrWhiteSpace(manifest.ProjectRoot)
                && !string.IsNullOrWhiteSpace(manifest.EngineRoot);
        }
        catch
        {
            return false;
        }
    }

    private static bool TryResolveFromEnvironment(out EngineLocation location)
    {
        location = null!;

        var projectRoot = Environment.GetEnvironmentVariable(ProjectRootEnvVar);
        var engineRoot = Environment.GetEnvironmentVariable(EngineRootEnvVar);

        if (!string.IsNullOrWhiteSpace(projectRoot) && Directory.Exists(projectRoot))
        {
            location = CreateLocation(projectRoot);
            return true;
        }

        if (!string.IsNullOrWhiteSpace(engineRoot) && Directory.Exists(engineRoot))
        {
            location = CreateLocation(Path.GetDirectoryName(engineRoot.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar))!);
            return true;
        }

        return false;
    }

    private static bool TryResolveFromInstallManifest(out EngineLocation location)
    {
        location = null!;
        if (!TryReadInstallManifest(out var manifest))
        {
            return false;
        }

        if (!Directory.Exists(manifest.ProjectRoot) || !Directory.Exists(manifest.EngineRoot))
        {
            return false;
        }

        var launcherRoot = string.IsNullOrWhiteSpace(manifest.LauncherRoot)
            ? manifest.ProjectRoot
            : manifest.LauncherRoot;

        location = new EngineLocation
        {
            ProjectRoot = Path.GetFullPath(manifest.ProjectRoot),
            EngineRoot = Path.GetFullPath(manifest.EngineRoot),
            LauncherRoot = Path.GetFullPath(launcherRoot),
            IgniteBtProjectPath = Path.Combine(manifest.EngineRoot, "Source", "Programs", "IgniteBT", "IgniteBT.csproj")
        };
        return true;
    }

    private static bool TryResolveFromLauncherLocation(string startDirectory, out EngineLocation location)
    {
        location = null!;
        var dir = new DirectoryInfo(startDirectory);

        while (dir != null)
        {
            if (IsRepositoryLauncherRoot(dir.FullName))
            {
                location = CreateLocation(dir.FullName);
                return true;
            }

            dir = dir.Parent;
        }

        return false;
    }

    private static bool IsRepositoryLauncherRoot(string directory)
    {
        return Directory.Exists(Path.Combine(directory, "Engine", "Source"))
            && (File.Exists(Path.Combine(directory, "we.ps1"))
                || File.Exists(Path.Combine(directory, "we")));
    }

    private static EngineLocation CreateLocation(string projectRoot)
    {
        projectRoot = Path.GetFullPath(projectRoot);
        var engineRoot = Path.Combine(projectRoot, "Engine");

        return new EngineLocation
        {
            ProjectRoot = projectRoot,
            EngineRoot = engineRoot,
            LauncherRoot = projectRoot,
            IgniteBtProjectPath = Path.Combine(engineRoot, "Source", "Programs", "IgniteBT", "IgniteBT.csproj")
        };
    }

    public sealed class InstallManifest
    {
        public string ProjectRoot { get; set; } = string.Empty;
        public string EngineRoot { get; set; } = string.Empty;
        public string LauncherRoot { get; set; } = string.Empty;

        // Legacy field from earlier installs.
        public string ScriptsRoot { get; set; } = string.Empty;

        public DateTime InstalledAtUtc { get; set; }
    }
}
