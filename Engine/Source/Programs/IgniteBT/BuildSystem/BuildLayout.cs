using IgniteBT.Compiler;

namespace IgniteBT.BuildSystem;

/// <summary>
/// Central build directory layout for IgniteBT.
/// All generated artifacts live under a single disposable Build/ tree at the project root.
/// </summary>
public sealed class BuildLayout
{
    public string ProjectRoot { get; }
    public string EngineRoot { get; }
    public string PlatformFolder { get; }
    public string ConfigurationFolder { get; }

    public BuildLayout(string projectRoot, string platform, BuildConfiguration configuration, TargetArchitecture architecture = TargetArchitecture.x64)
    {
        ProjectRoot = projectRoot;
        EngineRoot = Path.Combine(projectRoot, "Engine");
        PlatformFolder = GetPlatformFolderName(platform, architecture);
        ConfigurationFolder = GetConfigurationFolderName(configuration);
    }

    public string BuildRoot => Path.Combine(ProjectRoot, "Build");

    public string OutputRoot => Path.Combine(BuildRoot, "Output");
    public string PlatformOutputRoot => Path.Combine(OutputRoot, PlatformFolder, ConfigurationFolder);

    public string IntermediateRoot => Path.Combine(BuildRoot, "Intermediate");
    public string PlatformIntermediateRoot => Path.Combine(IntermediateRoot, PlatformFolder, ConfigurationFolder);
    public string ObjectsRoot => Path.Combine(PlatformIntermediateRoot, "Objects");
    public string ProgramDatabaseRoot => Path.Combine(PlatformIntermediateRoot, "PDB");
    public string IncrementalRoot => Path.Combine(PlatformIntermediateRoot, "Incremental");
    public string ModulesMetadataRoot => Path.Combine(PlatformIntermediateRoot, "Modules");

    public string GeneratedRoot => Path.Combine(BuildRoot, "Generated");
    public string PlatformGeneratedRoot => Path.Combine(GeneratedRoot, PlatformFolder, ConfigurationFolder);

    public string CacheDirectory => Path.Combine(BuildRoot, "Cache");
    public string DatabaseDirectory => Path.Combine(BuildRoot, "Database");
    public string LogsDirectory => Path.Combine(BuildRoot, "Logs");
    public string ReportsDirectory => Path.Combine(BuildRoot, "Reports");
    public string ManifestDirectory => Path.Combine(BuildRoot, "Manifest");
    public string TempRoot => Path.Combine(BuildRoot, "Temp");

    public string IgniteBtOutputRoot => Path.Combine(IntermediateRoot, "IgniteBT");

    public string GetModuleObjectsDirectory(string moduleName) =>
        Path.Combine(ObjectsRoot, moduleName);

    public string GetModuleGeneratedDirectory(string moduleName) =>
        Path.Combine(PlatformGeneratedRoot, moduleName);

    public string GetModuleProgramDatabasePath(string moduleName) =>
        Path.Combine(ProgramDatabaseRoot, moduleName + ".pdb");

    public string GetModuleIncrementalLinkPath(string moduleName) =>
        Path.Combine(IncrementalRoot, moduleName + ".ilk");

    public string GetModuleExportDefinitionPath(string moduleName) =>
        Path.Combine(GetModuleGeneratedDirectory(moduleName), moduleName + "_exports.def");

    public void EnsureDirectories()
    {
        Directory.CreateDirectory(OutputRoot);
        Directory.CreateDirectory(IntermediateRoot);
        Directory.CreateDirectory(GeneratedRoot);
        Directory.CreateDirectory(CacheDirectory);
        Directory.CreateDirectory(DatabaseDirectory);
        Directory.CreateDirectory(LogsDirectory);
        Directory.CreateDirectory(ReportsDirectory);
        Directory.CreateDirectory(ManifestDirectory);
        Directory.CreateDirectory(TempRoot);
        Directory.CreateDirectory(IgniteBtOutputRoot);

        Directory.CreateDirectory(ObjectsRoot);
        Directory.CreateDirectory(ProgramDatabaseRoot);
        Directory.CreateDirectory(IncrementalRoot);
        Directory.CreateDirectory(ModulesMetadataRoot);
        Directory.CreateDirectory(PlatformGeneratedRoot);
        Directory.CreateDirectory(PlatformOutputRoot);
    }

    public static string? FindProjectRoot(string startDir)
    {
        var dir = new DirectoryInfo(startDir);

        while (dir != null)
        {
            var engineSource = Path.Combine(dir.FullName, "Engine", "Source");
            if (Directory.Exists(engineSource))
            {
                return dir.FullName;
            }

            dir = dir.Parent;
        }

        return null;
    }

    public static string? FindEngineRoot(string startDir)
    {
        var projectRoot = FindProjectRoot(startDir);
        if (projectRoot == null)
        {
            return null;
        }

        var engineRoot = Path.Combine(projectRoot, "Engine");
        return Directory.Exists(Path.Combine(engineRoot, "Source")) ? engineRoot : null;
    }

    public static BuildLayout Resolve(string startDir, string platform, BuildConfiguration configuration)
    {
        var projectRoot = FindProjectRoot(startDir)
            ?? throw new InvalidOperationException($"Could not find project root from {startDir}");

        return new BuildLayout(projectRoot, platform, configuration);
    }

    public static string GetPlatformFolderName(string platform, TargetArchitecture architecture)
    {
        var arch = architecture switch
        {
            TargetArchitecture.ARM64 => "Arm64",
            TargetArchitecture.x86 => "Win32",
            _ => "Win64"
        };

        return platform.ToLowerInvariant() switch
        {
            "windows" => arch,
            "linux" => "Linux",
            "mac" or "macos" => "Mac",
            _ => arch
        };
    }

    public static string GetConfigurationFolderName(BuildConfiguration configuration) =>
        configuration switch
        {
            BuildConfiguration.Shipping => "Shipping",
            BuildConfiguration.Development => "Development",
            BuildConfiguration.Profile => "Profile",
            _ => "Debug"
        };

    public static IEnumerable<string> GetLegacyArtifactDirectories(string projectRoot, string engineRoot)
    {
        yield return Path.Combine(projectRoot, "Output");
        yield return Path.Combine(engineRoot, "Binaries");
        yield return Path.Combine(engineRoot, "Intermediate");
        yield return Path.Combine(projectRoot, "Logs");
        yield return Path.Combine(engineRoot, "Source", "Programs", "IgniteBT", "bin");
        yield return Path.Combine(engineRoot, "Source", "Programs", "IgniteBT", "obj");
        yield return Path.Combine(projectRoot, "Build", "cmake");
        yield return Path.Combine(projectRoot, "Build", "IgniteBT");
        yield return Path.Combine(projectRoot, "Build", "Win64");
        yield return Path.Combine(projectRoot, "Build", "Linux");
        yield return Path.Combine(projectRoot, "Build", "Mac");
        yield return Path.Combine(projectRoot, "Build", "CMakeFiles");
        yield return Path.Combine(projectRoot, "Build", "_deps");
        yield return Path.Combine(projectRoot, "Build", "bin");
        yield return Path.Combine(projectRoot, "Build", "Engine");
        yield return Path.Combine(projectRoot, "Build", "x64");
    }
}
