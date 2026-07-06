using System.Text.Json;
using IgniteBT.Core.Launcher;

namespace IgniteBT.Core.Launcher;

/// <summary>
/// Global installation metadata for the <c>we</c> command outside a repository checkout.
/// </summary>
public static class EngineInstallation
{
    private const string InstallFolderName = "WindEffects";
    private const string ManifestFileName = "engine.json";

    public sealed class EngineLocation
    {
        public required string ProjectRoot { get; init; }
        public required string EngineRoot { get; init; }
        public required string LauncherRoot { get; init; }
        public required EngineDescriptorData Descriptor { get; init; }
    }

    public static EngineLocation Resolve(string? startDirectory = null)
    {
        var descriptor = BootstrapLauncher.ResolveDescriptor(startDirectory);
        return new EngineLocation
        {
            ProjectRoot = descriptor.EngineRoot,
            EngineRoot = Path.Combine(descriptor.EngineRoot, "Engine"),
            LauncherRoot = descriptor.EngineRoot,
            Descriptor = descriptor
        };
    }

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

    public sealed class InstallManifest
    {
        public string ProjectRoot { get; set; } = string.Empty;
        public string EngineRoot { get; set; } = string.Empty;
        public string LauncherRoot { get; set; } = string.Empty;
        public DateTime InstalledAtUtc { get; set; }
    }
}
