namespace IgniteBT.Build.Layout;

/// <summary>
/// Where a built module's primary binary is published under the configuration output root.
/// Inferred from module metadata when left unspecified.
/// </summary>
public enum OutputPlacement
{
    Unspecified = 0,
    /// <summary>Bootstrap executables and bootstrap engine DLLs at the configuration root.</summary>
    ConfigurationRoot,
    /// <summary>Non-bootstrap engine modules under Engine/Binaries/.</summary>
    EngineBinary,
    /// <summary>Standalone tools under Programs/{ModuleName}/.</summary>
    Program,
    /// <summary>Plugin modules under Plugins/{ModuleName}/.</summary>
    Plugin
}

/// <summary>
/// Primary product directories under Build/Output/{Platform}/{Configuration}/.
/// </summary>
public static class OutputDirectories
{
    public const string Engine = "Engine";
    public const string EngineBinaries = "Engine/Binaries";
    public const string EngineContent = "Engine/Content";
    public const string EngineConfig = "Engine/Config";
    public const string ProductAssets = "Assets";
    public const string EngineResources = "Engine/Resources";
    public const string EngineShaders = "Engine/Shaders";
    public const string EngineData = "Engine/Data";
    public const string Programs = "Programs";
    public const string Plugins = "Plugins";
    public const string ThirdParty = "ThirdParty";
    public const string Saved = "Saved";
}

/// <summary>
/// Build-system metadata files written under Build/Manifest/{Platform}/{Configuration}/.
/// </summary>
public static class BuildManifestFiles
{
    public const string OutputLayout = "output-layout.json";
}
