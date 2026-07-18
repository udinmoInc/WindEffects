using Serilog;
using System.Collections.Concurrent;
using System.Linq;

namespace IgniteBT.Workspace.ThirdParty;

/// <summary>
/// Manages third-party library discovery and resolution.
/// </summary>
public class ThirdPartyManager
{
    private static readonly Lazy<ThirdPartyManager> _instance = new(() => new ThirdPartyManager());
    public static ThirdPartyManager Instance => _instance.Value;
    
    private readonly ConcurrentDictionary<string, ThirdPartyLibrary> _libraries = new();
    private readonly string _engineRoot;
    private readonly string _thirdPartyPath;
    private bool _initialized = false;
    
    private ThirdPartyManager()
    {
        _engineRoot = FindEngineRoot();
        _thirdPartyPath = Path.Combine(_engineRoot, "ThirdParty");
    }
    
    /// <summary>
    /// Initializes the third-party manager.
    /// </summary>
    public void Initialize()
    {
        if (_initialized) return;
        
        Log.Information("Initializing ThirdParty Manager...");
        
        if (!Directory.Exists(_thirdPartyPath))
        {
            Log.Warning("ThirdParty directory does not exist: {Path}", _thirdPartyPath);
            _initialized = true;
            return;
        }
        
        // Discover all third-party libraries
        DiscoverLibraries();
        
        _initialized = true;
        Log.Information("ThirdParty Manager initialized with {Count} libraries", _libraries.Count);
    }
    
    /// <summary>
    /// Discovers third-party libraries in the ThirdParty directory.
    /// </summary>
    private void DiscoverLibraries()
    {
        if (!Directory.Exists(_thirdPartyPath))
        {
            return;
        }
        
        var libraryDirs = Directory.GetDirectories(_thirdPartyPath);
        
        foreach (var libraryDir in libraryDirs)
        {
            var libraryName = Path.GetFileName(libraryDir);
            var library = CreateLibrary(libraryName, libraryDir);
            
            if (library != null)
            {
                _libraries[libraryName] = library;
                Log.Debug("Discovered third-party library: {Name}", libraryName);
            }
        }
    }
    
    /// <summary>
    /// Creates a third-party library from a directory.
    /// </summary>
    private ThirdPartyLibrary? CreateLibrary(string name, string path)
    {
        var library = new ThirdPartyLibrary
        {
            Name = name,
            RootPath = path,
            Version = DetectVersion(path)
        };
        
        // Detect include paths
        var includeDirs = new[]
        {
            Path.Combine(path, "include"),
            Path.Combine(path, "Include"),
            Path.Combine(path, "inc"),
            Path.Combine(path, "headers"),
            Path.Combine(path, "src"),
            path
        };
        
        foreach (var includeDir in includeDirs)
        {
            if (Directory.Exists(includeDir))
            {
                library.IncludePaths.Add(includeDir);
            }
        }
        
        // Detect library paths
        var libDirs = new[]
        {
            Path.Combine(path, "lib"),
            Path.Combine(path, "Lib"),
            Path.Combine(path, "libs"),
            Path.Combine(path, "Libraries")
        };
        
        foreach (var libDir in libDirs)
        {
            if (Directory.Exists(libDir))
            {
                library.LibraryPaths.Add(libDir);
                
                // Add architecture-specific subdirectories
                var archDirs = Directory.GetDirectories(libDir);
                foreach (var archDir in archDirs)
                {
                    library.LibraryPaths.Add(archDir);
                }
            }
        }
        
        // Detect binary paths
        var binDirs = new[]
        {
            Path.Combine(path, "bin"),
            Path.Combine(path, "Bin"),
            Path.Combine(path, "binary")
        };
        
        foreach (var binDir in binDirs)
        {
            if (Directory.Exists(binDir))
            {
                library.BinaryPaths.Add(binDir);
            }
        }
        
        // Detect library names from library files
        DetectLibraryNames(library);
        
        // Detect compiler definitions
        DetectCompilerDefinitions(library);
        
        // Validate
        ValidateLibrary(library);
        
        return library;
    }
    
    /// <summary>
    /// Detects the version of a third-party library.
    /// </summary>
    private string DetectVersion(string path)
    {
        // Try to find version file
        var versionFiles = new[]
        {
            Path.Combine(path, "version.txt"),
            Path.Combine(path, "VERSION"),
            Path.Combine(path, ".version"),
            Path.Combine(path, "CMakeLists.txt"),
            Path.Combine(path, "CMakeLists.txt")
        };
        
        foreach (var versionFile in versionFiles)
        {
            if (File.Exists(versionFile))
            {
                try
                {
                    var content = File.ReadAllText(versionFile);
                    // Simple regex to extract version
                    var match = System.Text.RegularExpressions.Regex.Match(
                        content, @"(\d+\.\d+\.\d+)");
                    if (match.Success)
                    {
                        return match.Groups[1].Value;
                    }
                }
                catch
                {
                    // Ignore file read errors
                }
            }
        }
        
        return "Unknown";
    }
    
    /// <summary>
    /// Detects library names from library files.
    /// </summary>
    private void DetectLibraryNames(ThirdPartyLibrary library)
    {
        foreach (var libPath in library.LibraryPaths)
        {
            if (!Directory.Exists(libPath))
            {
                continue;
            }
            
            var files = Directory.GetFiles(libPath, "*.lib", SearchOption.TopDirectoryOnly);
            foreach (var file in files)
            {
                var libName = Path.GetFileNameWithoutExtension(file);
                if (!library.LibraryNames.Contains(libName))
                {
                    library.LibraryNames.Add(libName);
                }
            }
            
            // Also check for .a files (Unix)
            var aFiles = Directory.GetFiles(libPath, "*.a", SearchOption.TopDirectoryOnly);
            foreach (var file in aFiles)
            {
                var libName = Path.GetFileNameWithoutExtension(file);
                if (!library.LibraryNames.Contains(libName))
                {
                    library.LibraryNames.Add(libName);
                }
            }
        }
    }
    
    /// <summary>
    /// Detects compiler definitions for the library.
    /// </summary>
    private void DetectCompilerDefinitions(ThirdPartyLibrary library)
    {
        // Generate standard definitions based on library name
        var def = library.Name.ToUpperInvariant().Replace("-", "_").Replace(".", "_");
        library.CompilerDefinitions.Add($"WE_HAS_{def}");
        library.CompilerDefinitions.Add($"WE_{def}_ENABLED");
        
        // Try to find a config file with definitions
        var configFile = Path.Combine(library.RootPath, "IgniteBT.config");
        if (File.Exists(configFile))
        {
            try
            {
                var lines = File.ReadAllLines(configFile);
                foreach (var line in lines)
                {
                    if (line.StartsWith("DEFINES:", StringComparison.OrdinalIgnoreCase))
                    {
                        var definitions = line.Substring(8).Split(',');
                        foreach (var definition in definitions)
                        {
                            var trimmed = definition.Trim();
                            if (!string.IsNullOrEmpty(trimmed))
                            {
                                library.CompilerDefinitions.Add(trimmed);
                            }
                        }
                    }
                }
            }
            catch
            {
                // Ignore file read errors
            }
        }
    }
    
    /// <summary>
    /// Validates a third-party library.
    /// </summary>
    private void ValidateLibrary(ThirdPartyLibrary library)
    {
        library.IsValid = true;

        if (library.IncludePaths.Count == 0)
        {
            library.Warnings.Add("No include paths found");
            library.IsValid = false;
        }

        // Header-only libraries must expose their primary include.
        if (string.Equals(library.Name, "glm", StringComparison.OrdinalIgnoreCase))
        {
            var hasPrimaryHeader = library.IncludePaths.Any(includePath =>
                File.Exists(Path.Combine(includePath, "glm", "glm.hpp")));
            if (!hasPrimaryHeader)
            {
                library.Warnings.Add("Missing primary header glm/glm.hpp");
                library.IsValid = false;
            }
        }

        if (string.Equals(library.Name, "SDL3", StringComparison.OrdinalIgnoreCase))
        {
            var hasPrimaryHeader = library.IncludePaths.Any(includePath =>
                File.Exists(Path.Combine(includePath, "SDL3", "SDL.h")));
            if (!hasPrimaryHeader)
            {
                library.Warnings.Add("Missing primary header SDL3/SDL.h");
                library.IsValid = false;
            }
        }

        if (library.LibraryPaths.Count == 0)
        {
            library.Warnings.Add("No library paths found");
        }

        if (library.LibraryNames.Count == 0)
        {
            library.Warnings.Add("No library files found");
        }
    }
    
    /// <summary>
    /// Gets a third-party library by name.
    /// </summary>
    public ThirdPartyLibrary? GetLibrary(string name)
    {
        Initialize();
        
        if (_libraries.TryGetValue(name, out var library))
        {
            return library;
        }
        
        return null;
    }
    
    /// <summary>
    /// Gets all third-party libraries.
    /// </summary>
    public Dictionary<string, ThirdPartyLibrary> GetAllLibraries()
    {
        Initialize();
        return new Dictionary<string, ThirdPartyLibrary>(_libraries);
    }
    
    /// <summary>
    /// Checks if a library is available.
    /// </summary>
    public bool HasLibrary(string name)
    {
        Initialize();
        return _libraries.ContainsKey(name);
    }
    
    private string FindEngineRoot()
    {
        // BuildLayout.FindEngineRoot returns {repo}/Engine — the build-system engine directory.
        return IgniteBT.Build.Layout.BuildLayout.FindEngineRoot(Directory.GetCurrentDirectory())
            ?? Directory.GetCurrentDirectory();
    }
}
