namespace IgniteBT.Build.Linker;

/// <summary>
/// Link target type.
/// </summary>
public enum LinkTargetType
{
    /// <summary>
    /// Executable.
    /// </summary>
    Executable,

    /// <summary>
    /// Shared library (DLL/so/dylib).
    /// </summary>
    SharedLibrary,

    /// <summary>
    /// Static library (lib/a).
    /// </summary>
    StaticLibrary
}

/// <summary>
/// Link result.
/// </summary>
public class LinkResult
{
    /// <summary>
    /// Whether linking succeeded.
    /// </summary>
    public bool Success { get; set; }

    /// <summary>
    /// Output file path.
    /// </summary>
    public string OutputFile { get; set; } = string.Empty;

    /// <summary>
    /// Link time in milliseconds.
    /// </summary>
    public long LinkTimeMs { get; set; }

    /// <summary>
    /// Standard error output from linker.
    /// </summary>
    public string StandardError { get; set; } = string.Empty;

    /// <summary>
    /// Standard output from linker.
    /// </summary>
    public string StandardOutput { get; set; } = string.Empty;

    /// <summary>
    /// Exit code from linker process.
    /// </summary>
    public int ExitCode { get; set; }
}

/// <summary>
/// Linker options for a link task.
/// </summary>
public class LinkerOptions
{
    /// <summary>
    /// Object files to link.
    /// </summary>
    public List<string> ObjectFiles { get; set; } = new();

    /// <summary>
    /// Libraries to link against.
    /// </summary>
    public List<string> Libraries { get; set; } = new();

    /// <summary>
    /// Library search paths.
    /// </summary>
    public List<string> LibraryDirectories { get; set; } = new();

    /// <summary>
    /// Output file path.
    /// </summary>
    public string OutputFile { get; set; } = string.Empty;

    /// <summary>
    /// Link target type.
    /// </summary>
    public LinkTargetType TargetType { get; set; }

    /// <summary>
    /// Build configuration.
    /// </summary>
    public Compiler.BuildConfiguration Configuration { get; set; }

    /// <summary>
    /// Target platform.
    /// </summary>
    public Compiler.TargetPlatform Platform { get; set; }

    /// <summary>
    /// Target architecture.
    /// </summary>
    public Compiler.TargetArchitecture Architecture { get; set; }

    /// <summary>
    /// Additional linker flags.
    /// </summary>
    public List<string> AdditionalFlags { get; set; } = new();

    /// <summary>
    /// Whether to generate debug information.
    /// </summary>
    public bool GenerateDebugInfo { get; set; }

    /// <summary>
    /// Whether to enable incremental linking.
    /// </summary>
    public bool EnableIncrementalLinking { get; set; }

    /// <summary>
    /// Working directory for linking.
    /// </summary>
    public string WorkingDirectory { get; set; } = string.Empty;

    /// <summary>
    /// Import library output path (for DLLs on Windows).
    /// </summary>
    public string? ImportLibrary { get; set; }

    /// <summary>
    /// Export definition file (for DLLs on Windows).
    /// </summary>
    public string? DefinitionFile { get; set; }

    /// <summary>
    /// Export all symbols from object files (Windows shared libraries).
    /// </summary>
    public bool ExportAllSymbols { get; set; }

    /// <summary>
    /// Program database output path (PDB on Windows).
    /// </summary>
    public string? ProgramDatabaseFile { get; set; }

    /// <summary>
    /// Incremental linker database output path (ILK on Windows).
    /// </summary>
    public string? IncrementalLinkDatabaseFile { get; set; }

    /// <summary>
    /// Windows subsystem override (Console, Windows).
    /// </summary>
    public string? Subsystem { get; set; }
}

/// <summary>
/// Interface for linker implementations.
/// </summary>
public interface ILinker
{
    /// <summary>
    /// Linker name (e.g., "MSVC LINK", "LLD", "GNU ld").
    /// </summary>
    string Name { get; }

    /// <summary>
    /// Linker executable path.
    /// </summary>
    string ExecutablePath { get; }

    /// <summary>
    /// Linker version string.
    /// </summary>
    string Version { get; }

    /// <summary>
    /// Target platform this linker supports.
    /// </summary>
    Compiler.TargetPlatform Platform { get; }

    /// <summary>
    /// Detects if the linker is installed and available.
    /// </summary>
    bool IsAvailable();

    /// <summary>
    /// Detects the linker version.
    /// </summary>
    Task<string> DetectVersionAsync();

    /// <summary>
    /// Links object files into the target output.
    /// </summary>
    Task<LinkResult> LinkAsync(LinkerOptions options);

    /// <summary>
    /// Creates a static library from object files.
    /// </summary>
    Task<LinkResult> CreateStaticLibraryAsync(LinkerOptions options);

    /// <summary>
    /// Gets the default linker flags for a configuration.
    /// </summary>
    List<string> GetDefaultFlags(Compiler.BuildConfiguration configuration, LinkTargetType targetType);

    /// <summary>
    /// Gets the subsystem flags for the target type.
    /// </summary>
    List<string> GetSubsystemFlags(LinkTargetType targetType);
}
