using Serilog;
using IgniteBT.ModuleDiscovery;
using IgniteBT.BuildGraph;
using IgniteBT.Compiler;
using IgniteBT.Dependency;
using IgniteBT.BuildCacheSystem;
using IgniteBT.Scheduler;
using IgniteBT.Linker;
using IgniteBT.Toolchain;
using IgniteBT.SDK;
using IgniteBT.Dependencies;
using IgniteBT.ThirdParty;
using BuildTaskScheduler = IgniteBT.Scheduler.TaskScheduler;

namespace IgniteBT.Commands;

public static class BuildCommand
{
    public static async Task<int> Execute(string[] args)
    {
        var target = args.Length > 0 ? args[0] : "All";
        var config = GetArgValue(args, "--config", "Release");
        var platform = GetArgValue(args, "--platform", GetCurrentPlatform());
        var clean = HasArg(args, "--clean");
        var jobs = int.Parse(GetArgValue(args, "--jobs", Environment.ProcessorCount.ToString()));

        Log.Information("Build Command");
        Log.Information("Target: {Target}", target);
        Log.Information("Configuration: {Config}", config);
        Log.Information("Platform: {Platform}", platform);
        Log.Information("Clean: {Clean}", clean);
        Log.Information("Jobs: {Jobs}", jobs);

        try
        {
            // Get engine directory
            var currentDir = Directory.GetCurrentDirectory();
            var engineDir = FindEngineRoot(currentDir);
            
            if (string.IsNullOrEmpty(engineDir) || !Directory.Exists(engineDir))
            {
                Log.Error("Could not find engine root directory from {CurrentDir}", currentDir);
                return 1;
            }
            
            Log.Information("Engine root: {EngineDir}", engineDir);
            
            // Detect and setup toolchain once for all modules
            var detectedCompiler = ToolchainDetector.DetectCompiler();
            Log.Information("Detected compiler: {Type} v{Version} at {Path}", detectedCompiler.Type, detectedCompiler.Version, detectedCompiler.Path);

            // Resolve all dependencies (SDKs, third-party libraries)
            var dependencyResolver = new DependencyResolver();
            var dependencyResult = await dependencyResolver.ResolveAsync();
            
            // Log dependency resolution results
            foreach (var warning in dependencyResult.Warnings)
            {
                Log.Warning(warning);
            }
            
            foreach (var error in dependencyResult.Errors)
            {
                Log.Error(error);
            }
            
            if (!dependencyResult.Success)
            {
                Log.Error("Dependency resolution failed");
                return 1;
            }
            
            Log.Information("Resolved {SDKCount} SDKs and {LibCount} third-party libraries", 
                dependencyResult.SDKs.Count, dependencyResult.ThirdPartyLibraries.Count);
            Log.Information("Generated {FlagCount} feature flags", dependencyResult.FeatureFlags.Count);
            
            // Parse configuration
            var buildConfig = ParseConfiguration(config);
            
            // Discover modules
            var discovery = new ModuleDiscoverer(engineDir);
            discovery.SetDependencyResult(dependencyResult);
            var modules = await discovery.DiscoverModulesAsync();
            
            if (modules.Count == 0)
            {
                Log.Warning("No modules found to build");
                return 0;
            }
            
            // Build dependency graph
            var graph = new DependencyGraph();
            graph.BuildFromModules(modules);
            
            // Validate graph
            if (!graph.Validate())
            {
                Log.Error("Build graph validation failed");
                return 1;
            }
            
            // Get build order
            var buildOrder = graph.GetBuildOrder();
            
            // Initialize build cache
            var cacheDir = Path.Combine(engineDir, "Intermediate", "IgniteBT", "Cache");
            var cache = new BuildCache(cacheDir);
            
            // Initialize dependency scanner
            var depScanner = new DependencyScanner();
            
            // Initialize task scheduler
            var scheduler = new BuildTaskScheduler(jobs);
            
            // Create build tasks for each module
            foreach (var node in buildOrder)
            {
                var task = new BuildTask
                {
                    Id = node.Name,
                    Name = $"Build {node.Name}",
                    Dependencies = node.Dependencies.Select(d => d.Name).ToList(),
                    ModuleNode = node,
                    Work = () => BuildModule(node, buildConfig, platform, engineDir, cache, depScanner, detectedCompiler, dependencyResult, graph)
                };
                
                scheduler.AddTask(task);
            }
            
            // Execute build
            var success = await scheduler.ExecuteAsync();
            
            // Print statistics
            var stats = scheduler.GetStats();
            Log.Information("Build Statistics:");
            Log.Information("  Total Tasks: {Total}", stats.TotalTasks);
            Log.Information("  Completed: {Completed}", stats.CompletedTasks);
            Log.Information("  Failed: {Failed}", stats.FailedTasks);
            Log.Information("  Duration: {Duration}ms", stats.TotalDurationMs);
            
            // Print cache statistics
            var cacheStats = cache.GetStats();
            Log.Information("Cache Statistics:");
            Log.Information("  Entries: {Count}", cacheStats.EntryCount);
            Log.Information("  Size: {Size:F2} MB", cacheStats.TotalSizeMB);
            
            scheduler.Dispose();
            
            return success ? 0 : 1;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Build failed with exception");
            return 1;
        }
    }

    private static async Task BuildModule(BuildNode node, BuildConfiguration config, string platform, 
        string engineDir, BuildCache cache, DependencyScanner depScanner, DetectedCompiler detectedCompiler, DependencyResolutionResult dependencyResult, DependencyGraph buildGraph)
    {
        Log.Information("Building module: {ModuleName}", node.Name);
        
        // Get module directory from the discovered module
        var moduleDir = node.Module.ModuleDirectory;
        if (string.IsNullOrEmpty(moduleDir) || !Directory.Exists(moduleDir))
        {
            Log.Warning("Module directory does not exist: {ModuleDir}", moduleDir);
            return;
        }
        
        Log.Information("Module directory: {ModuleDir}", moduleDir);
        
        // Discover source files
        var sourceFiles = DiscoverSourceFiles(moduleDir);
        Log.Information("Found {Count} source files in module {ModuleName}", sourceFiles.Count, node.Name);
        
        if (sourceFiles.Count == 0)
        {
            Log.Warning("No source files found for module {ModuleName}", node.Name);
            return;
        }

        // Select compiler based on detected toolchain
        ICompiler compiler = detectedCompiler.Type switch
        {
            CompilerType.MSVC => new MSVCCompiler(),
            CompilerType.GCC => new GCCCompiler(),
            CompilerType.Clang => new ClangCompiler(),
            _ => new MSVCCompiler()
        };

        // Set the compiler path if detected
        if (!string.IsNullOrEmpty(detectedCompiler.Path) && compiler is MSVCCompiler msvcCompiler)
        {
            msvcCompiler.SetExecutablePath(detectedCompiler.Path);
        }

        // Set the vcvarsall.bat path if available
        if (!string.IsNullOrEmpty(detectedCompiler.VcVarsAllPath) && compiler is MSVCCompiler msvcCompilerWithVcVars)
        {
            msvcCompilerWithVcVars.SetVcVarsAllPath(detectedCompiler.VcVarsAllPath);
        }

        // Select linker based on detected toolchain
        ILinker linker = detectedCompiler.Type switch
        {
            CompilerType.MSVC => new MSVCLinker(),
            CompilerType.GCC => new GNULinker(),
            CompilerType.Clang => new LLDLinker(),
            _ => new MSVCLinker()
        };

        // Set linker executable path if available
        if (linker is MSVCLinker msvcLinker && !string.IsNullOrEmpty(detectedCompiler.Path))
        {
            var linkPath = Path.Combine(Path.GetDirectoryName(detectedCompiler.Path)!, "link.exe");
            Log.Debug("Attempting to set linker path: {LinkPath}", linkPath);
            if (File.Exists(linkPath))
            {
                msvcLinker.SetExecutablePath(linkPath);
                Log.Information("Set linker executable path: {LinkPath}", linkPath);
            }
            else
            {
                Log.Warning("link.exe not found at: {LinkPath}", linkPath);
            }

            // Pass vcvarsall.bat information to linker
            if (!string.IsNullOrEmpty(detectedCompiler.VcVarsAllPath))
            {
                msvcLinker.SetVcVarsAllPath(detectedCompiler.VcVarsAllPath);
                Log.Debug("Set linker vcvarsall.bat path: {VcVarsPath}", detectedCompiler.VcVarsAllPath);
            }
        }
        
        // Check linker availability
        if (!linker.IsAvailable())
        {
            Log.Warning("Linker {LinkerName} is not available - skipping linking", linker.Name);
            Log.Information("Module {ModuleName} build skipped (linker not available)", node.Name);
            return;
        }
        
        // Check compiler availability
        if (detectedCompiler.Type == CompilerType.None || !compiler.IsAvailable())
        {
            Log.Warning("Compiler {CompilerName} is not available - skipping actual compilation", compiler.Name);
            Log.Information("Module {ModuleName} build skipped (dry run mode)", node.Name);
            return;
        }
        
        // Detect compiler version
        var compilerVersion = await compiler.DetectVersionAsync();
        Log.Information("Using compiler: {CompilerName} v{Version}", compiler.Name, compilerVersion);
        
        // Create output directory
        var outputDir = Path.Combine(engineDir, "Intermediate", node.Name);
        Directory.CreateDirectory(outputDir);
        
        // Compile each source file
        var objectFiles = new List<string>();
        var compiledCount = 0;
        var cachedCount = 0;
        
        foreach (var sourceFile in sourceFiles)
        {
            var objectFile = Path.Combine(outputDir, Path.GetFileNameWithoutExtension(sourceFile) + ".obj");
            
            // Scan dependencies
            var dependencies = await depScanner.ScanFileAsync(sourceFile, transitive: true);
            
            // Compute cache key
            var cacheKey = cache.ComputeCacheKey(sourceFile, dependencies.IncludedHeaders, 
                new CompilerOptions { Configuration = config }, compilerVersion);
            
            // Check cache
            if (cache.TryGetCachedObject(cacheKey, out var cachedObjectFile) && !string.IsNullOrEmpty(cachedObjectFile))
            {
                // Copy from cache
                File.Copy(cachedObjectFile, objectFile, true);
                cachedCount++;
                Log.Debug("Cache hit for {SourceFile}", Path.GetFileName(sourceFile));
            }
            else
            {
                // Compile
                var compileOptions = new CompilerOptions
                {
                    SourceFile = sourceFile,
                    OutputFile = objectFile,
                    Configuration = config,
                    Platform = IgniteBT.Compiler.TargetPlatform.Windows,
                    Architecture = IgniteBT.Compiler.TargetArchitecture.x64,
                    CppStandard = "C++20",
                    IncludeDirectories = new List<string>
                    {
                        Path.Combine(engineDir, "Source"),
                        Path.Combine(moduleDir, "Public")
                    },
                    GenerateDebugInfo = config == BuildConfiguration.Debug,
                    EnableOptimizations = config != BuildConfiguration.Debug
                };

                // Add SDK include paths
                foreach (var (sdkName, sdkInfo) in dependencyResult.SDKs)
                {
                    foreach (var includePath in sdkInfo.IncludePaths)
                    {
                        if (!compileOptions.IncludeDirectories.Contains(includePath))
                        {
                            compileOptions.IncludeDirectories.Add(includePath);
                        }
                    }
                }
                
                // Add third-party library include paths
                foreach (var (libName, libInfo) in dependencyResult.ThirdPartyLibraries)
                {
                    foreach (var includePath in libInfo.IncludePaths)
                    {
                        if (!compileOptions.IncludeDirectories.Contains(includePath))
                        {
                            compileOptions.IncludeDirectories.Add(includePath);
                        }
                    }
                }
                
                // Add module dependency public include paths
                foreach (var depName in node.Module.PublicDependencies.Concat(node.Module.PrivateDependencies))
                {
                    var depModule = buildGraph.Nodes.FirstOrDefault(n => n.Name == depName);
                    if (depModule != null)
                    {
                        var depPublicInclude = Path.Combine(engineDir, "Source", depModule.Module.ModuleDirectory, "Public");
                        if (Directory.Exists(depPublicInclude) && !compileOptions.IncludeDirectories.Contains(depPublicInclude))
                        {
                            compileOptions.IncludeDirectories.Add(depPublicInclude);
                        }
                    }
                }
                
                // Add feature flags as compiler definitions
                foreach (var (flagName, flagValue) in dependencyResult.FeatureFlags)
                {
                    var definition = $"{flagName}={flagValue}";
                    if (!compileOptions.Definitions.Contains(definition))
                    {
                        compileOptions.Definitions.Add(definition);
                    }
                }
                
                // Add module-specific definitions
                foreach (var definition in node.Module.Definitions)
                {
                    if (!compileOptions.Definitions.Contains(definition))
                    {
                        compileOptions.Definitions.Add(definition);
                    }
                }
                
                var result = await compiler.CompileAsync(compileOptions);
                
                if (!result.Success)
                {
                    Log.Error("Failed to compile {SourceFile}: {Error}", sourceFile, result.StandardError);
                    throw new InvalidOperationException($"Compilation failed for {sourceFile}");
                }
                
                // Cache the object file
                cache.CacheObject(cacheKey, objectFile, result.CompilationTimeMs);
                compiledCount++;
                Log.Debug("Compiled {SourceFile} in {Time}ms", Path.GetFileName(sourceFile), result.CompilationTimeMs);
            }
            
            objectFiles.Add(objectFile);
        }
        
        Log.Information("Module {ModuleName}: Compiled {Compiled} files, {Cached} from cache", 
            node.Name, compiledCount, cachedCount);
        
        // Link the module
        var linkOutputDir = Path.Combine(engineDir, "Binaries", config.ToString());
        Directory.CreateDirectory(linkOutputDir);
        
        var linkOutputFile = Path.Combine(linkOutputDir, node.Name + ".dll");
        
        var linkOptions = new LinkerOptions
        {
            ObjectFiles = objectFiles,
            OutputFile = linkOutputFile,
            TargetType = LinkTargetType.SharedLibrary,
            Configuration = config,
            Platform = ParsePlatform(platform),
            Architecture = IgniteBT.Compiler.TargetArchitecture.x64,
            GenerateDebugInfo = config == BuildConfiguration.Debug,
            WorkingDirectory = outputDir,
            LibraryDirectories = new List<string>(),
            Libraries = new List<string>()
        };
        
        // Add Windows SDK library paths from dependency resolution
        if (dependencyResult.SDKs.TryGetValue("WindowsSDK", out var windowsSdkInfo))
        {
            foreach (var libPath in windowsSdkInfo.LibraryPaths)
            {
                linkOptions.LibraryDirectories.Add(libPath);
                linkOptions.LibraryDirectories.Add(Path.Combine(libPath, "um"));
                linkOptions.LibraryDirectories.Add(Path.Combine(libPath, "ucrt"));
            }
        }
        
        // Add dbghelp.lib for crash reporting
        linkOptions.Libraries.Add("dbghelp.lib");
        linkOptions.Libraries.Add("psapi.lib");
        
        var linkResult = await linker.LinkAsync(linkOptions);
        
        if (!linkResult.Success)
        {
            Log.Error("Failed to link module {ModuleName}: {Error}", node.Name, linkResult.StandardError);
            throw new InvalidOperationException($"Linking failed for module {node.Name}");
        }
        
        Log.Information("Module {ModuleName} linked successfully in {Time}ms", node.Name, linkResult.LinkTimeMs);
        Log.Information("Module {ModuleName} built successfully", node.Name);
    }

    private static List<string> DiscoverSourceFiles(string moduleDir)
    {
        var sourceFiles = new List<string>();
        
        // Search for .cpp files
        sourceFiles.AddRange(Directory.GetFiles(moduleDir, "*.cpp", SearchOption.AllDirectories));
        
        // Search for .cxx files
        sourceFiles.AddRange(Directory.GetFiles(moduleDir, "*.cxx", SearchOption.AllDirectories));
        
        // Search for .cc files
        sourceFiles.AddRange(Directory.GetFiles(moduleDir, "*.cc", SearchOption.AllDirectories));
        
        return sourceFiles;
    }

    private static IgniteBT.Compiler.TargetPlatform ParsePlatform(string platform)
    {
        return platform.ToLowerInvariant() switch
        {
            "windows" => IgniteBT.Compiler.TargetPlatform.Windows,
            "linux" => IgniteBT.Compiler.TargetPlatform.Linux,
            "mac" or "macos" => IgniteBT.Compiler.TargetPlatform.MacOS,
            _ => IgniteBT.Compiler.TargetPlatform.Windows
        };
    }

    private static BuildConfiguration ParseConfiguration(string config)
    {
        return config.ToLowerInvariant() switch
        {
            "debug" => BuildConfiguration.Debug,
            "development" or "dev" => BuildConfiguration.Development,
            "profile" => BuildConfiguration.Profile,
            "shipping" or "release" => BuildConfiguration.Shipping,
            _ => BuildConfiguration.Shipping
        };
    }

    static string GetArgValue(string[] args, string name, string defaultValue)
    {
        for (int i = 0; i < args.Length; i++)
        {
            if (args[i] == name && i + 1 < args.Length)
                return args[i + 1];
        }
        return defaultValue;
    }

    static bool HasArg(string[] args, string name)
    {
        return args.Contains(name);
    }

    static string GetCurrentPlatform()
    {
        if (OperatingSystem.IsWindows())
            return "Windows";
        if (OperatingSystem.IsLinux())
            return "Linux";
        if (OperatingSystem.IsMacOS())
            return "Mac";
        return "Unknown";
    }

    static string? FindEngineRoot(string startDir)
    {
        var dir = new DirectoryInfo(startDir);
        
        while (dir != null)
        {
            if (dir.Name.Equals("Engine", StringComparison.OrdinalIgnoreCase))
            {
                var sourceDir = Path.Combine(dir.FullName, "Source");
                if (Directory.Exists(sourceDir))
                {
                    return dir.FullName;
                }
            }
            
            var engineParent = Path.Combine(dir.FullName, "Engine", "Source");
            if (Directory.Exists(engineParent))
            {
                return Path.GetDirectoryName(engineParent);
            }
            
            dir = dir.Parent;
        }
        
        return null;
    }
}
