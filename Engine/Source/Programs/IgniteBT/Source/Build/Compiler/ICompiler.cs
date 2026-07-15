namespace IgniteBT.Build.Compiler;

/// <summary>
/// Build configuration types.
/// </summary>
public enum BuildConfiguration
{
    /// <summary>
    /// Debug configuration - no optimizations, full debug info.
    /// </summary>
    Debug,

    /// <summary>
    /// Development configuration - some optimizations, debug info.
    /// </summary>
    Development,

    /// <summary>
    /// Profile configuration - optimizations, profiling info.
    /// </summary>
    Profile,

    /// <summary>
    /// Shipping configuration - full optimizations, no debug info.
    /// </summary>
    Shipping
}

/// <summary>
/// Target platform.
/// </summary>
public enum TargetPlatform
{
    Windows,
    Linux,
    MacOS,
    Unknown
}

/// <summary>
/// Target architecture.
/// </summary>
public enum TargetArchitecture
{
    x64,
    ARM64,
    x86,
    Unknown
}

/// <summary>
/// Compiler diagnostic message.
/// </summary>
public class CompilerDiagnostic
{
    /// <summary>
    /// Diagnostic severity.
    /// </summary>
    public DiagnosticSeverity Severity { get; set; }

    /// <summary>
    /// Source file path.
    /// </summary>
    public string FilePath { get; set; } = string.Empty;

    /// <summary>
    /// Line number (1-based).
    /// </summary>
    public int Line { get; set; }

    /// <summary>
    /// Column number (1-based).
    /// </summary>
    public int Column { get; set; }

    /// <summary>
    /// Diagnostic code (e.g., C1234).
    /// </summary>
    public string Code { get; set; } = string.Empty;

    /// <summary>
    /// Diagnostic message.
    /// </summary>
    public string Message { get; set; } = string.Empty;
}

/// <summary>
/// Diagnostic severity levels.
/// </summary>
public enum DiagnosticSeverity
{
    Info,
    Warning,
    Error,
    Fatal
}

/// <summary>
/// Compilation result.
/// </summary>
public class CompilationResult
{
    /// <summary>
    /// Whether compilation succeeded.
    /// </summary>
    public bool Success { get; set; }

    /// <summary>
    /// Output object file path.
    /// </summary>
    public string OutputFile { get; set; } = string.Empty;

    /// <summary>
    /// Compilation time in milliseconds.
    /// </summary>
    public long CompilationTimeMs { get; set; }

    /// <summary>
    /// Diagnostics generated during compilation.
    /// </summary>
    public List<CompilerDiagnostic> Diagnostics { get; set; } = new();

    /// <summary>
    /// Standard error output from compiler.
    /// </summary>
    public string StandardError { get; set; } = string.Empty;

    /// <summary>
    /// Standard output from compiler.
    /// </summary>
    public string StandardOutput { get; set; } = string.Empty;

    /// <summary>
    /// Exit code from compiler process.
    /// </summary>
    public int ExitCode { get; set; }
}

/// <summary>
/// Compiler options for a compilation task.
/// </summary>
public class CompilerOptions
{
    /// <summary>
    /// Source file to compile.
    /// </summary>
    public string SourceFile { get; set; } = string.Empty;

    /// <summary>
    /// Output object file path.
    /// </summary>
    public string OutputFile { get; set; } = string.Empty;

    /// <summary>
    /// Include directories.
    /// </summary>
    public List<string> IncludeDirectories { get; set; } = new();

    /// <summary>
    /// Preprocessor definitions.
    /// </summary>
    public List<string> Definitions { get; set; } = new();

    /// <summary>
    /// Additional compiler flags.
    /// </summary>
    public List<string> AdditionalFlags { get; set; } = new();

    /// <summary>
    /// Build configuration.
    /// </summary>
    public BuildConfiguration Configuration { get; set; }

    /// <summary>
    /// Target platform.
    /// </summary>
    public TargetPlatform Platform { get; set; }

    /// <summary>
    /// Target architecture.
    /// </summary>
    public TargetArchitecture Architecture { get; set; }

    /// <summary>
    /// C++ standard version.
    /// </summary>
    public string CppStandard { get; set; } = "c++23";

    /// <summary>
    /// Whether to generate debug information.
    /// </summary>
    public bool GenerateDebugInfo { get; set; }

    /// <summary>
    /// Whether to enable optimizations.
    /// </summary>
    public bool EnableOptimizations { get; set; }

    /// <summary>
    /// Precompiled header file (optional).
    /// </summary>
    public string? PrecompiledHeader { get; set; }

    /// <summary>
    /// Whether this is a precompiled header compilation.
    /// </summary>
    public bool IsPrecompiledHeader { get; set; }

    /// <summary>
    /// Working directory for compilation.
    /// </summary>
    public string WorkingDirectory { get; set; } = string.Empty;
}

/// <summary>
/// Interface for compiler implementations.
/// </summary>
public interface ICompiler
{
    /// <summary>
    /// Compiler name (e.g., "MSVC", "Clang", "GCC").
    /// </summary>
    string Name { get; }

    /// <summary>
    /// Compiler executable path.
    /// </summary>
    string ExecutablePath { get; }

    /// <summary>
    /// Compiler version string.
    /// </summary>
    string Version { get; }

    /// <summary>
    /// Target platform this compiler supports.
    /// </summary>
    TargetPlatform Platform { get; }

    /// <summary>
    /// Detects if the compiler is installed and available.
    /// </summary>
    bool IsAvailable();

    /// <summary>
    /// Detects the compiler version.
    /// </summary>
    Task<string> DetectVersionAsync();

    /// <summary>
    /// Compiles a single source file.
    /// </summary>
    Task<CompilationResult> CompileAsync(CompilerOptions options);

    /// <summary>
    /// Generates a precompiled header.
    /// </summary>
    Task<CompilationResult> GeneratePrecompiledHeaderAsync(CompilerOptions options);

    /// <summary>
    /// Gets the default compiler flags for a configuration.
    /// </summary>
    List<string> GetDefaultFlags(BuildConfiguration configuration);

    /// <summary>
    /// Gets the default optimization flags for a configuration.
    /// </summary>
    List<string> GetOptimizationFlags(BuildConfiguration configuration);

    /// <summary>
    /// Gets the default warning flags.
    /// </summary>
    List<string> GetWarningFlags();
}
