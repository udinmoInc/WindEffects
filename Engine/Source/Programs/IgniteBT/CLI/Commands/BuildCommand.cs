using Serilog;
using IgniteBT.Workspace.Modules;
using IgniteBT.Build.Layout;
using IgniteBT.Build.Shaders;
using IgniteBT.Build.Graph;
using IgniteBT.Build.Compiler;
using IgniteBT.Build.Dependencies;
using IgniteBT.Core.Cache;
using IgniteBT.Build.Scheduler;
using IgniteBT.Build.Linker;
using IgniteBT.Build.Toolchain;
using IgniteBT.Workspace.SDK;
using IgniteBT.Workspace.ThirdParty;
using BuildTaskScheduler = IgniteBT.Build.Scheduler.TaskScheduler;

namespace IgniteBT.CLI;

public static class BuildCommand
{
    public static async Task<int> Execute(string[] args)
    {
        var parsed = CommandSchemas.Build.Parse(args);
        if (!CommandLineHelpers.TryReportErrors(parsed))
        {
            return 1;
        }

        var target = parsed.ResolveTarget();
        var config = parsed.GetOption("config", "Release");
        var platform = CommandLineHelpers.NormalizePlatform(parsed.GetOption("platform", string.Empty));
        var clean = parsed.HasFlag("clean");

        int jobs;
        try
        {
            jobs = CommandLineHelpers.ParseJobs(parsed.GetOption("jobs", string.Empty));
        }
        catch (ArgumentException ex)
        {
            Log.Error("{Message}", ex.Message);
            return 1;
        }

        Log.Information("Build Command");
        Log.Information("Target: {Target}", target);
        Log.Information("Configuration: {Config}", config);
        Log.Information("Platform: {Platform}", platform);
        Log.Information("Clean: {Clean}", clean);
        Log.Information("Jobs: {Jobs}", jobs);

        try
        {
            var currentDir = Directory.GetCurrentDirectory();
            var projectRoot = BuildLayout.FindProjectRoot(currentDir);
            var engineDir = BuildLayout.FindEngineRoot(currentDir);
            
            if (string.IsNullOrEmpty(projectRoot) || string.IsNullOrEmpty(engineDir) || !Directory.Exists(engineDir))
            {
                Log.Error("Could not find project root directory from {CurrentDir}", currentDir);
                return 1;
            }
            
            Log.Information("Project root: {ProjectRoot}", projectRoot);
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
            var buildConfig = CommandLineHelpers.ParseConfiguration(config);
            var layout = BuildLayout.Resolve(currentDir, platform, buildConfig);
            layout.EnsureDirectories();
            
            // Discover modules
            var discovery = new ModuleDiscoverer(engineDir, config, platform);
            discovery.SetDependencyResult(dependencyResult);
            var modules = await discovery.DiscoverModulesAsync();
            
            if (modules.Count == 0)
            {
                Log.Warning("No modules found to build");
                return 0;
            }

            var outputLayout = new OutputLayout(layout, engineDir);
            outputLayout.RegisterModules(modules);
            outputLayout.PrepareModuleDirectories(modules);
            outputLayout.StageEngineAssets(modules);
            ShaderBytecodeCompiler.CompileAndStage(engineDir, outputLayout.ConfigurationRoot);
            
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
            var cache = new BuildCache(layout.CacheDirectory);
            
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
                    Work = () => BuildModule(node, buildConfig, platform, engineDir, layout, outputLayout, cache, depScanner, detectedCompiler, dependencyResult, graph)
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

            outputLayout.PruneConfigurationRoot(modules);
            outputLayout.RefreshLayoutManifest(modules);
            outputLayout.StageRuntimeDependencies(dependencyResult, modules);
            
            return success ? 0 : 1;
        }
        catch (ModuleDiscoveryException ex)
        {
            Log.Error(ex, "Module discovery failed");
            return 1;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Build failed with exception");
            return 1;
        }
    }

    private static async Task BuildModule(BuildNode node, BuildConfiguration config, string platform, 
        string engineDir, BuildLayout layout, OutputLayout outputLayout, BuildCache cache, DependencyScanner depScanner, DetectedCompiler detectedCompiler, DependencyResolutionResult dependencyResult, DependencyGraph buildGraph)
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
        var objectDir = layout.GetModuleObjectsDirectory(node.Name);
        var generatedDir = layout.GetModuleGeneratedDirectory(node.Name);
        Directory.CreateDirectory(objectDir);
        Directory.CreateDirectory(generatedDir);
        
        // Compile each source file
        var objectFiles = new List<string>();
        var compiledCount = 0;
        var cachedCount = 0;
        
        foreach (var sourceFile in sourceFiles)
        {
            var objectFile = Path.Combine(objectDir, Path.GetFileNameWithoutExtension(sourceFile) + ".obj");
            
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
                    Platform = IgniteBT.Build.Compiler.TargetPlatform.Windows,
                    Architecture = IgniteBT.Build.Compiler.TargetArchitecture.x64,
                    CppStandard = "C++20",
                    IncludeDirectories = new List<string>
                    {
                        Path.Combine(engineDir, "Source"),
                        Path.Combine(moduleDir, "Public")
                    },
                    GenerateDebugInfo = config == BuildConfiguration.Debug,
                    EnableOptimizations = config != BuildConfiguration.Debug
                };

                // Add module's private include paths
                Log.Debug("Module {Module} has {Count} private include paths", node.Name, node.Module.PrivateIncludePaths.Count);
                foreach (var privateIncludePath in node.Module.PrivateIncludePaths)
                {
                    Log.Debug("Processing private include path: {Path}", privateIncludePath);
                    var fullPath = Path.Combine(moduleDir, privateIncludePath);
                    Log.Debug("Full path: {FullPath}", fullPath);
                    Log.Debug("Directory exists: {Exists}", Directory.Exists(fullPath));
                    if (Directory.Exists(fullPath) && !compileOptions.IncludeDirectories.Contains(fullPath))
                    {
                        compileOptions.IncludeDirectories.Add(fullPath);
                        Log.Debug("Added private include path: {Path}", fullPath);
                    }
                    else
                    {
                        Log.Warning("Private include path does not exist or already added: {Path}", fullPath);
                    }
                }

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
                    var depNode = buildGraph.GetNode(depName);
                    if (depNode == null)
                    {
                        continue;
                    }

                    var depModuleDir = depNode.Module.ModuleDirectory;
                    foreach (var publicIncludePath in depNode.Module.PublicIncludePaths)
                    {
                        var fullPath = Path.Combine(depModuleDir, publicIncludePath);
                        if (Directory.Exists(fullPath) && !compileOptions.IncludeDirectories.Contains(fullPath))
                        {
                            compileOptions.IncludeDirectories.Add(fullPath);
                        }
                    }

                    var defaultPublicInclude = Path.Combine(depModuleDir, "Public");
                    if (Directory.Exists(defaultPublicInclude)
                        && !compileOptions.IncludeDirectories.Contains(defaultPublicInclude))
                    {
                        compileOptions.IncludeDirectories.Add(defaultPublicInclude);
                    }
                }

                if (node.Module.PlatformSettings.Windows is { } windowsCompileSettings)
                {
                    compileOptions.AdditionalFlags.AddRange(windowsCompileSettings.CompilerFlags);
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
        outputLayout.EnsureModuleDirectories(node.Module);
        var linkOutputFile = outputLayout.GetModuleBinaryPath(node.Module);
        var linkOutputDir = Path.GetDirectoryName(linkOutputFile)!;
        Directory.CreateDirectory(linkOutputDir);
        Directory.CreateDirectory(layout.GetModuleImportLibraryDirectory(node.Name));
        Directory.CreateDirectory(layout.ProgramDatabaseRoot);
        Directory.CreateDirectory(layout.IncrementalRoot);

        var linkTargetType = node.Module.Type switch
        {
            ModuleType.Executable => LinkTargetType.Executable,
            ModuleType.StaticLibrary => LinkTargetType.StaticLibrary,
            _ => LinkTargetType.SharedLibrary
        };
        
        var linkOptions = new LinkerOptions
        {
            ObjectFiles = objectFiles,
            OutputFile = linkOutputFile,
            ImportLibrary = linkTargetType != LinkTargetType.StaticLibrary
                ? outputLayout.GetImportLibraryPath(node.Module)
                : null,
            TargetType = linkTargetType,
            Configuration = config,
            Platform = ParsePlatform(platform),
            Architecture = IgniteBT.Build.Compiler.TargetArchitecture.x64,
            GenerateDebugInfo = config == BuildConfiguration.Debug,
            ExportAllSymbols = node.Module.Type == ModuleType.SharedLibrary,
            WorkingDirectory = generatedDir,
            ProgramDatabaseFile = config == BuildConfiguration.Debug
                ? layout.GetModuleProgramDatabasePath(node.Name)
                : null,
            IncrementalLinkDatabaseFile = config == BuildConfiguration.Debug && linkTargetType != LinkTargetType.StaticLibrary
                ? layout.GetModuleIncrementalLinkPath(node.Name)
                : null,
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

            foreach (var winLib in new[] { "kernel32.lib", "user32.lib", "gdi32.lib", "ole32.lib", "shell32.lib" })
            {
                if (!linkOptions.Libraries.Contains(winLib))
                {
                    linkOptions.Libraries.Add(winLib);
                }
            }
        }
        
        // Add dbghelp.lib for crash reporting
        linkOptions.Libraries.Add("dbghelp.lib");
        linkOptions.Libraries.Add("psapi.lib");

        if (node.Module.PlatformSettings.Windows is { } windowsLinkSettings)
        {
            linkOptions.Subsystem = windowsLinkSettings.Subsystem;
            linkOptions.AdditionalFlags.AddRange(windowsLinkSettings.LinkerFlags);
        }

        // Link SDK libraries requested by this module
        foreach (var sdkName in node.Module.RequiredSDKs.Concat(node.Module.OptionalSDKs))
        {
            if (!dependencyResult.SDKs.TryGetValue(sdkName, out var sdkInfo) || !sdkInfo.IsValid)
            {
                continue;
            }

            foreach (var libPath in sdkInfo.LibraryPaths)
            {
                if (!linkOptions.LibraryDirectories.Contains(libPath))
                {
                    linkOptions.LibraryDirectories.Add(libPath);
                }
            }

            if (sdkName.Equals("SDL3", StringComparison.OrdinalIgnoreCase))
            {
                var sdlLib = sdkInfo.LibraryPaths
                    .SelectMany(dir => Directory.Exists(dir) ? Directory.GetFiles(dir, "SDL3*.lib") : Array.Empty<string>())
                    .FirstOrDefault(path =>
                    {
                        var libFile = Path.GetFileName(path);
                        return libFile.Equals("SDL3-static.lib", StringComparison.OrdinalIgnoreCase)
                            || libFile.Equals("SDL3.lib", StringComparison.OrdinalIgnoreCase);
                    });

                if (sdlLib != null)
                {
                    var libFile = Path.GetFileName(sdlLib);
                    if (!linkOptions.Libraries.Contains(libFile))
                    {
                        linkOptions.Libraries.Add(libFile);
                    }
                }
            }
            else if (sdkName.Equals("VulkanSDK", StringComparison.OrdinalIgnoreCase))
            {
                var vulkanLib = sdkInfo.LibraryPaths
                    .SelectMany(dir => Directory.Exists(dir) ? Directory.GetFiles(dir, "vulkan-1.lib") : Array.Empty<string>())
                    .FirstOrDefault();

                if (vulkanLib != null)
                {
                    const string libFile = "vulkan-1.lib";
                    if (!linkOptions.Libraries.Contains(libFile))
                    {
                        linkOptions.Libraries.Add(libFile);
                    }
                }
            }
        }

        // Link against module dependency import libraries
        var seenLibDirectories = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var depName in node.Module.PublicDependencies.Concat(node.Module.PrivateDependencies))
        {
            var depNode = buildGraph.GetNode(depName);
            if (depNode == null)
            {
                Log.Warning("Dependency module not found in graph for {Module}: {Dependency}", node.Name, depName);
                continue;
            }

            var depLibPath = outputLayout.GetImportLibraryPath(depNode.Module);
            var depLibDir = Path.GetDirectoryName(depLibPath)!;
            var depLibFile = Path.GetFileName(depLibPath);

            if (seenLibDirectories.Add(depLibDir))
            {
                linkOptions.LibraryDirectories.Add(depLibDir);
            }

            if (File.Exists(depLibPath))
            {
                if (!linkOptions.Libraries.Contains(depLibFile))
                {
                    linkOptions.Libraries.Add(depLibFile);
                }
            }
            else
            {
                Log.Warning("Dependency import library not found for module {Module}: {Lib}", node.Name, depLibPath);
            }
        }

        if (linkTargetType == LinkTargetType.Executable
            && ParsePlatform(platform) == IgniteBT.Build.Compiler.TargetPlatform.Windows)
        {
            AddDelayLoadLibraries(linkOptions, node, buildGraph, outputLayout);
        }

        if (ParsePlatform(platform) == IgniteBT.Build.Compiler.TargetPlatform.Windows)
        {
            AddThirdPartyDelayLoadLibraries(linkOptions);
        }
        
        var linkResult = await linker.LinkAsync(linkOptions);
        
        if (!linkResult.Success)
        {
            Log.Error("Failed to link module {ModuleName}: {Error}", node.Name, linkResult.StandardError);
            throw new InvalidOperationException($"Linking failed for module {node.Name}");
        }

        outputLayout.PruneConfigurationRoot(buildGraph.Nodes.Select(n => n.Module));
        
        Log.Information(
            "Module {ModuleName} linked to {OutputPath} in {Time}ms",
            node.Name,
            linkOutputFile,
            linkResult.LinkTimeMs);
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

    private static void AddDelayLoadLibraries(
        LinkerOptions linkOptions,
        BuildNode node,
        DependencyGraph buildGraph,
        OutputLayout outputLayout)
    {
        if (!linkOptions.Libraries.Contains("delayimp.lib", StringComparer.OrdinalIgnoreCase))
        {
            linkOptions.Libraries.Add("delayimp.lib");
        }

        foreach (var depName in node.Module.PublicDependencies.Concat(node.Module.PrivateDependencies))
        {
            var depNode = buildGraph.GetNode(depName);
            if (depNode == null || depNode.Module.IsBootstrapBinary)
            {
                continue;
            }

            if (depNode.Module.Type != ModuleType.SharedLibrary)
            {
                continue;
            }

            var binaryName = Path.GetFileName(outputLayout.GetModuleBinaryPath(depNode.Module));
            AddDelayLoadFlag(linkOptions, binaryName);
        }

        if (linkOptions.Libraries.Any(lib =>
                lib.StartsWith("SDL3", StringComparison.OrdinalIgnoreCase)
                && lib.EndsWith(".lib", StringComparison.OrdinalIgnoreCase)))
        {
            AddDelayLoadFlag(linkOptions, "SDL3.dll");
        }

        if (linkOptions.Libraries.Contains("vulkan-1.lib", StringComparer.OrdinalIgnoreCase))
        {
            AddDelayLoadFlag(linkOptions, "vulkan-1.dll");
        }
    }

    private static void AddThirdPartyDelayLoadLibraries(LinkerOptions linkOptions)
    {
        if (!linkOptions.Libraries.Contains("delayimp.lib", StringComparer.OrdinalIgnoreCase)
            && linkOptions.Libraries.Any(lib =>
                (lib.StartsWith("SDL3", StringComparison.OrdinalIgnoreCase)
                 || lib.Equals("vulkan-1.lib", StringComparison.OrdinalIgnoreCase))
                && lib.EndsWith(".lib", StringComparison.OrdinalIgnoreCase)))
        {
            linkOptions.Libraries.Add("delayimp.lib");
        }

        if (linkOptions.Libraries.Any(lib =>
                lib.StartsWith("SDL3", StringComparison.OrdinalIgnoreCase)
                && lib.EndsWith(".lib", StringComparison.OrdinalIgnoreCase)))
        {
            AddDelayLoadFlag(linkOptions, "SDL3.dll");
        }

        if (linkOptions.Libraries.Contains("vulkan-1.lib", StringComparer.OrdinalIgnoreCase))
        {
            AddDelayLoadFlag(linkOptions, "vulkan-1.dll");
        }
    }

    private static void AddDelayLoadFlag(LinkerOptions linkOptions, string dllName)
    {
        var flag = $"/DELAYLOAD:{dllName}";
        if (!linkOptions.AdditionalFlags.Contains(flag, StringComparer.OrdinalIgnoreCase))
        {
            linkOptions.AdditionalFlags.Add(flag);
        }
    }

    private static IgniteBT.Build.Compiler.TargetPlatform ParsePlatform(string platform)
    {
        return platform.ToLowerInvariant() switch
        {
            "windows" or "win64" or "win32" => IgniteBT.Build.Compiler.TargetPlatform.Windows,
            "linux" => IgniteBT.Build.Compiler.TargetPlatform.Linux,
            "mac" or "macos" => IgniteBT.Build.Compiler.TargetPlatform.MacOS,
            _ => IgniteBT.Build.Compiler.TargetPlatform.Windows
        };
    }
}
