using IgniteBT.Build.Dependencies;
using IgniteBT.Build.Graph;

namespace IgniteBT.Build.Compiler;

/// <summary>
/// Builds consistent include paths and compiler options for compilation,
/// dependency scanning, and incremental cache keys.
/// </summary>
public static class ModuleCompileEnvironment
{
    private static string ResolveIncludePath(string moduleDir, string includePath) =>
        Path.IsPathRooted(includePath) ? includePath : Path.Combine(moduleDir, includePath);

    private static void AddIncludeDirectory(List<string> includeDirectories, string fullPath)
    {
        if (Directory.Exists(fullPath) && !includeDirectories.Contains(fullPath))
        {
            includeDirectories.Add(fullPath);
        }
    }

    public static List<string> CollectIncludeDirectories(
        BuildNode node,
        string engineDir,
        string moduleDir,
        DependencyResolutionResult dependencyResult,
        DependencyGraph buildGraph)
    {
        var includeDirectories = new List<string>
        {
            Path.Combine(engineDir, "Source")
        };

        foreach (var publicIncludePath in node.Module.PublicIncludePaths)
        {
            AddIncludeDirectory(includeDirectories, ResolveIncludePath(moduleDir, publicIncludePath));
        }

        if (!includeDirectories.Contains(Path.Combine(moduleDir, "Public")))
        {
            AddIncludeDirectory(includeDirectories, Path.Combine(moduleDir, "Public"));
        }

        foreach (var privateIncludePath in node.Module.PrivateIncludePaths)
        {
            AddIncludeDirectory(includeDirectories, ResolveIncludePath(moduleDir, privateIncludePath));
        }

        foreach (var sdkInfo in dependencyResult.SDKs.Values)
        {
            foreach (var includePath in sdkInfo.IncludePaths)
            {
                if (!includeDirectories.Contains(includePath))
                {
                    includeDirectories.Add(includePath);
                }
            }
        }

        foreach (var library in dependencyResult.ThirdPartyLibraries.Values)
        {
            foreach (var includePath in library.IncludePaths)
            {
                if (!includeDirectories.Contains(includePath))
                {
                    includeDirectories.Add(includePath);
                }
            }
        }

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
                AddIncludeDirectory(includeDirectories, ResolveIncludePath(depModuleDir, publicIncludePath));
            }

            AddIncludeDirectory(includeDirectories, Path.Combine(depModuleDir, "Public"));
        }

        return includeDirectories;
    }

    public static DependencyScanner CreateConfiguredScanner(IEnumerable<string> includeDirectories)
    {
        var scanner = new DependencyScanner();
        foreach (var includePath in includeDirectories)
        {
            if (Directory.Exists(includePath))
            {
                scanner.AddIncludePath(includePath);
            }
        }

        return scanner;
    }

    public static CompilerOptions CreateCompileOptions(
        BuildNode node,
        BuildConfiguration config,
        string sourceFile,
        string objectFile,
        List<string> includeDirectories,
        DependencyResolutionResult dependencyResult,
        IEnumerable<string> moduleDefinitions)
    {
        var compileOptions = new CompilerOptions
        {
            SourceFile = sourceFile,
            OutputFile = objectFile,
            Configuration = config,
            Platform = TargetPlatform.Windows,
            Architecture = TargetArchitecture.x64,
            CppStandard = "c++23",
            IncludeDirectories = new List<string>(includeDirectories),
            GenerateDebugInfo = config == BuildConfiguration.Debug,
            EnableOptimizations = config != BuildConfiguration.Debug
        };

        if (node.Module.PlatformSettings.Windows is { } windowsCompileSettings)
        {
            compileOptions.AdditionalFlags.AddRange(windowsCompileSettings.CompilerFlags);
        }

        foreach (var (flagName, flagValue) in dependencyResult.FeatureFlags)
        {
            var definition = $"{flagName}={flagValue}";
            if (!compileOptions.Definitions.Contains(definition))
            {
                compileOptions.Definitions.Add(definition);
            }
        }

        foreach (var definition in moduleDefinitions)
        {
            if (!compileOptions.Definitions.Contains(definition))
            {
                compileOptions.Definitions.Add(definition);
            }
        }

        return compileOptions;
    }
}
