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
    /// Constructor called by the build system.
    /// </summary>
    protected ModuleRules(ModuleContext context)
    {
        Context = context;
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

    public ModuleContext(string engineDirectory, string configuration, string platform, string architecture)
    {
        EngineDirectory = engineDirectory;
        Configuration = configuration;
        Platform = platform;
        Architecture = architecture;
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
