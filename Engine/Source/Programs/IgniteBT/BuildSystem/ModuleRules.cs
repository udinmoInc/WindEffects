namespace IgniteBT.BuildSystem;

/// <summary>
/// Base class for module Build.cs files. Each module extends this class to describe its build configuration.
/// </summary>
public abstract class ModuleRules
{
    /// <summary>
    /// Context provided to the module during initialization.
    /// </summary>
    protected ModuleContext Context { get; }

    /// <summary>
    /// Module name (auto-detected from file name).
    /// </summary>
    public string Name { get; internal set; } = string.Empty;

    /// <summary>
    /// Module directory path.
    /// </summary>
    public string ModuleDirectory { get; internal set; } = string.Empty;

    /// <summary>
    /// Type of module (Executable, SharedLibrary, StaticLibrary, etc.).
    /// </summary>
    public ModuleType Type { get; set; } = ModuleType.SharedLibrary;

    /// <summary>
    /// Public dependencies (other modules this module depends on).
    /// </summary>
    public List<string> PublicDependencies { get; } = new();

    /// <summary>
    /// Private dependencies (other modules this module depends on internally).
    /// </summary>
    public List<string> PrivateDependencies { get; } = new();

    /// <summary>
    /// Public include paths (exposed to dependent modules).
    /// </summary>
    public List<string> PublicIncludePaths { get; } = new();

    /// <summary>
    /// Private include paths (internal to this module).
    /// </summary>
    public List<string> PrivateIncludePaths { get; } = new();

    /// <summary>
    /// Source files to compile.
    /// </summary>
    public List<string> SourceFiles { get; } = new();

    /// <summary>
    /// Precompiled header file name (optional).
    /// </summary>
    public string? PrecompiledHeader { get; set; }

    /// <summary>
    /// Precompiled header source file (optional).
    /// </summary>
    public string? PrecompiledHeaderSource { get; set; }

    /// <summary>
    /// Compiler definitions.
    /// </summary>
    public List<string> Definitions { get; } = new();

    /// <summary>
    /// Linker libraries.
    /// </summary>
    public List<string> Libraries { get; } = new();

    /// <summary>
    /// Output directory (relative to build output).
    /// </summary>
    public string? OutputDirectory { get; set; }

    /// <summary>
    /// Whether this module is a plugin.
    /// </summary>
    public bool IsPlugin { get; set; }

    /// <summary>
    /// Platform-specific settings.
    /// </summary>
    public PlatformSettings PlatformSettings { get; } = new();

    /// <summary>
    /// Required SDKs for this module.
    /// </summary>
    public List<string> RequiredSDKs { get; } = new();

    /// <summary>
    /// Optional SDKs for this module.
    /// </summary>
    public List<string> OptionalSDKs { get; } = new();

    /// <summary>
    /// Required third-party libraries for this module.
    /// </summary>
    public List<string> RequiredThirdParty { get; } = new();

    /// <summary>
    /// Optional third-party libraries for this module.
    /// </summary>
    public List<string> OptionalThirdParty { get; } = new();

    /// <summary>
    /// Whether this module should be disabled.
    /// </summary>
    public bool IsDisabled { get; private set; }

    /// <summary>
    /// Constructor called by the build system.
    /// </summary>
    protected ModuleRules(ModuleContext context)
    {
        Context = context;
    }

    /// <summary>
    /// Requires an SDK to be available for this module to build.
    /// </summary>
    protected void RequireSDK(string sdkName)
    {
        if (!RequiredSDKs.Contains(sdkName))
        {
            RequiredSDKs.Add(sdkName);
        }
    }

    /// <summary>
    /// Optionally requires an SDK. The module can build without it, but features may be disabled.
    /// </summary>
    protected void OptionalSDK(string sdkName)
    {
        if (!OptionalSDKs.Contains(sdkName))
        {
            OptionalSDKs.Add(sdkName);
        }
    }

    /// <summary>
    /// Requires a module dependency.
    /// </summary>
    protected void RequireModule(string moduleName)
    {
        if (!PublicDependencies.Contains(moduleName))
        {
            PublicDependencies.Add(moduleName);
        }
    }

    /// <summary>
    /// Optionally requires a module dependency.
    /// </summary>
    protected void OptionalModule(string moduleName)
    {
        if (!PrivateDependencies.Contains(moduleName))
        {
            PrivateDependencies.Add(moduleName);
        }
    }

    /// <summary>
    /// Requires a third-party library.
    /// </summary>
    protected void RequireThirdParty(string libraryName)
    {
        if (!RequiredThirdParty.Contains(libraryName))
        {
            RequiredThirdParty.Add(libraryName);
        }
    }

    /// <summary>
    /// Optionally requires a third-party library.
    /// </summary>
    protected void AddOptionalThirdParty(string libraryName)
    {
        if (!OptionalThirdParty.Contains(libraryName))
        {
            OptionalThirdParty.Add(libraryName);
        }
    }

    /// <summary>
    /// Checks if an SDK is available.
    /// </summary>
    protected bool HasSDK(string sdkName)
    {
        return Context.HasSDK(sdkName);
    }

    /// <summary>
    /// Checks if a third-party library is available.
    /// </summary>
    protected bool HasThirdParty(string libraryName)
    {
        return Context.HasThirdParty(libraryName);
    }

    /// <summary>
    /// Enables this module only if the condition is true.
    /// </summary>
    protected void EnableIf(bool condition)
    {
        if (!condition)
        {
            IsDisabled = true;
        }
    }

    /// <summary>
    /// Disables this module.
    /// </summary>
    protected void DisableModule()
    {
        IsDisabled = true;
    }

    /// <summary>
    /// Adds a compiler definition if a condition is met.
    /// </summary>
    protected void DefineIf(bool condition, string definition)
    {
        if (condition && !Definitions.Contains(definition))
        {
            Definitions.Add(definition);
        }
    }
}

/// <summary>
/// Module type enumeration.
/// </summary>
public enum ModuleType
{
    /// <summary>
    /// Executable program.
    /// </summary>
    Executable,

    /// <summary>
    /// Shared library (DLL on Windows, .so on Linux, .dylib on Mac).
    /// </summary>
    SharedLibrary,

    /// <summary>
    /// Static library (.lib on Windows, .a on Linux/Mac).
    /// </summary>
    StaticLibrary,

    /// <summary>
    /// Object file collection.
    /// </summary>
    Object,

    /// <summary>
    /// Custom build target.
    /// </summary>
    Custom
}

/// <summary>
/// Context provided to module rules during initialization.
/// </summary>
public class ModuleContext
{
    /// <summary>
    /// Engine root directory.
    /// </summary>
    public string EngineDirectory { get; }

    /// <summary>
    /// Current build configuration (Debug, Release, etc.).
    /// </summary>
    public string Configuration { get; }

    /// <summary>
    /// Target platform (Windows, Linux, Mac).
    /// </summary>
    public string Platform { get; }

    /// <summary>
    /// Target architecture (x64, ARM64, etc.).
    /// </summary>
    public string Architecture { get; }

    /// <summary>
    /// Available SDKs.
    /// </summary>
    private Dictionary<string, bool> AvailableSDKs { get; } = new();

    /// <summary>
    /// Available third-party libraries.
    /// </summary>
    private Dictionary<string, bool> AvailableThirdParty { get; } = new();

    /// <summary>
    /// Feature flags.
    /// </summary>
    private Dictionary<string, string> FeatureFlags { get; } = new();

    public ModuleContext(string engineDirectory, string configuration, string platform, string architecture)
    {
        EngineDirectory = engineDirectory;
        Configuration = configuration;
        Platform = platform;
        Architecture = architecture;
    }

    /// <summary>
    /// Sets the available SDKs.
    /// </summary>
    internal void SetAvailableSDKs(Dictionary<string, bool> sdks)
    {
        foreach (var (name, available) in sdks)
        {
            AvailableSDKs[name] = available;
        }
    }

    /// <summary>
    /// Sets the available third-party libraries.
    /// </summary>
    internal void SetAvailableThirdParty(Dictionary<string, bool> libraries)
    {
        foreach (var (name, available) in libraries)
        {
            AvailableThirdParty[name] = available;
        }
    }

    /// <summary>
    /// Sets the feature flags.
    /// </summary>
    internal void SetFeatureFlags(Dictionary<string, string> flags)
    {
        foreach (var (name, value) in flags)
        {
            FeatureFlags[name] = value;
        }
    }

    /// <summary>
    /// Checks if an SDK is available.
    /// </summary>
    public bool HasSDK(string sdkName)
    {
        return AvailableSDKs.TryGetValue(sdkName, out var available) && available;
    }

    /// <summary>
    /// Checks if a third-party library is available.
    /// </summary>
    public bool HasThirdParty(string libraryName)
    {
        return AvailableThirdParty.TryGetValue(libraryName, out var available) && available;
    }

    /// <summary>
    /// Gets a feature flag value.
    /// </summary>
    public string? GetFeatureFlag(string flagName)
    {
        return FeatureFlags.TryGetValue(flagName, out var value) ? value : null;
    }

    /// <summary>
    /// Checks if a feature flag is set.
    /// </summary>
    public bool HasFeatureFlag(string flagName)
    {
        return FeatureFlags.ContainsKey(flagName);
    }
}

/// <summary>
/// Platform-specific build settings.
/// </summary>
public class PlatformSettings
{
    /// <summary>
    /// Windows-specific settings.
    /// </summary>
    public WindowsSettings? Windows { get; set; }

    /// <summary>
    /// Linux-specific settings.
    /// </summary>
    public LinuxSettings? Linux { get; set; }

    /// <summary>
    /// Mac-specific settings.
    /// </summary>
    public MacSettings? Mac { get; set; }
}

/// <summary>
/// Windows-specific build settings.
/// </summary>
public class WindowsSettings
{
    /// <summary>
    /// Additional compiler flags for MSVC.
    /// </summary>
    public List<string> CompilerFlags { get; } = new();

    /// <summary>
    /// Additional linker flags.
    /// </summary>
    public List<string> LinkerFlags { get; } = new();

    /// <summary>
    /// Windows subsystem (Console, Windows, etc.).
    /// </summary>
    public string? Subsystem { get; set; }
}

/// <summary>
/// Linux-specific build settings.
/// </summary>
public class LinuxSettings
{
    /// <summary>
    /// Additional compiler flags for GCC/Clang.
    /// </summary>
    public List<string> CompilerFlags { get; } = new();

    /// <summary>
    /// Additional linker flags.
    /// </summary>
    public List<string> LinkerFlags { get; } = new();
}

/// <summary>
/// Mac-specific build settings.
/// </summary>
public class MacSettings
{
    /// <summary>
    /// Additional compiler flags for Clang.
    /// </summary>
    public List<string> CompilerFlags { get; } = new();

    /// <summary>
    /// Additional linker flags.
    /// </summary>
    public List<string> LinkerFlags { get; } = new();

    /// <summary>
    /// Minimum macOS version.
    /// </summary>
    public string? MinVersion { get; set; }
}
