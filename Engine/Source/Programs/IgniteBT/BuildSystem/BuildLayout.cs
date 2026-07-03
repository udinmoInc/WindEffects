using IgniteBT.Compiler;

namespace IgniteBT.BuildSystem;

/// <summary>
/// Central build/output directory layout for IgniteBT.
/// All generated artifacts live under Build/ and Output/ at the project root.
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
    public string OutputRoot => Path.Combine(ProjectRoot, "Output");

    public string LogsDirectory => Path.Combine(BuildRoot, "Logs");
    public string CacheDirectory => Path.Combine(BuildRoot, "Cache");
    public string DatabaseDirectory => Path.Combine(BuildRoot, "Database");

    public string PlatformBuildRoot => Path.Combine(BuildRoot, PlatformFolder, ConfigurationFolder);
    public string PlatformOutputRoot => Path.Combine(OutputRoot, PlatformFolder, ConfigurationFolder);

    public string ObjectsRoot => Path.Combine(PlatformBuildRoot, "Objects");
    public string GeneratedRoot => Path.Combine(PlatformBuildRoot, "Generated");
    public string ModulesRoot => Path.Combine(PlatformBuildRoot, "Modules");
    public string ProgramDatabaseRoot => Path.Combine(PlatformBuildRoot, "PDB");
    public string IncrementalRoot => Path.Combine(PlatformBuildRoot, "Incremental");
    public string TempRoot => Path.Combine(PlatformBuildRoot, "Temp");

    public string GetModuleObjectsDirectory(string moduleName) =>
        Path.Combine(ObjectsRoot, moduleName);

    public string GetModuleGeneratedDirectory(string moduleName) =>
        Path.Combine(GeneratedRoot, moduleName);

    public string GetModuleOutputPath(string moduleName, string extension) =>
        Path.Combine(PlatformOutputRoot, moduleName + extension);

    public string GetModuleProgramDatabasePath(string moduleName) =>
        Path.Combine(ProgramDatabaseRoot, moduleName + ".pdb");

    public string GetModuleIncrementalLinkPath(string moduleName) =>
        Path.Combine(IncrementalRoot, moduleName + ".ilk");

    public string GetModuleExportDefinitionPath(string moduleName) =>
        Path.Combine(GetModuleGeneratedDirectory(moduleName), moduleName + "_exports.def");

    public void EnsureDirectories()
    {
        Directory.CreateDirectory(LogsDirectory);
        Directory.CreateDirectory(CacheDirectory);
        Directory.CreateDirectory(DatabaseDirectory);
        Directory.CreateDirectory(ObjectsRoot);
        Directory.CreateDirectory(GeneratedRoot);
        Directory.CreateDirectory(ModulesRoot);
        Directory.CreateDirectory(ProgramDatabaseRoot);
        Directory.CreateDirectory(IncrementalRoot);
        Directory.CreateDirectory(TempRoot);
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
            BuildConfiguration.Shipping => "Release",
            BuildConfiguration.Development => "Development",
            BuildConfiguration.Profile => "Profile",
            _ => "Debug"
        };

    public static IEnumerable<string> GetLegacyArtifactDirectories(string projectRoot, string engineRoot)
    {
        yield return Path.Combine(engineRoot, "Binaries");
        yield return Path.Combine(engineRoot, "Intermediate");
        yield return Path.Combine(projectRoot, "Logs");
        yield return Path.Combine(engineRoot, "Source", "Programs", "IgniteBT", "bin");
        yield return Path.Combine(engineRoot, "Source", "Programs", "IgniteBT", "obj");
        yield return Path.Combine(projectRoot, "Build", "cmake");
    }
}
