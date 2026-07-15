using IgniteBT.Build.Compiler;
using IgniteBT.Build.Dependencies;
using IgniteBT.Build.Graph;
using IgniteBT.Build.Layout;
using IgniteBT.Build.Linker;
using IgniteBT.Workspace.Modules;

namespace IgniteBT.Build.Orchestration;

/// <summary>
/// Constructs linker options — shared between orchestrator and legacy build path.
/// </summary>
public static class ModuleLinkEnvironment
{
    public static LinkerOptions CreateLinkOptions(
        BuildNode node,
        List<string> objectFiles,
        string linkOutputFile,
        BuildConfiguration config,
        string platform,
        BuildLayout layout,
        OutputLayout outputLayout,
        DependencyResolutionResult dependencyResult,
        DependencyGraph buildGraph,
        string engineDir)
    {
        var linkTargetType = node.Module.Type switch
        {
            ModuleType.Executable => LinkTargetType.Executable,
            ModuleType.StaticLibrary => LinkTargetType.StaticLibrary,
            _ => LinkTargetType.SharedLibrary
        };

        var generatedDir = layout.GetModuleGeneratedDirectory(node.Name);
        Directory.CreateDirectory(generatedDir);
        Directory.CreateDirectory(layout.GetModuleImportLibraryDirectory(node.Name));
        Directory.CreateDirectory(layout.ProgramDatabaseRoot);
        Directory.CreateDirectory(layout.IncrementalRoot);

        var linkOptions = new LinkerOptions
        {
            ObjectFiles = objectFiles,
            OutputFile = linkOutputFile,
            ImportLibrary = linkTargetType != LinkTargetType.StaticLibrary
                ? outputLayout.GetImportLibraryPath(node.Module) : null,
            TargetType = linkTargetType,
            Configuration = config,
            Platform = ParsePlatform(platform),
            Architecture = TargetArchitecture.x64,
            GenerateDebugInfo = config is BuildConfiguration.Debug or BuildConfiguration.Development,
            ExportAllSymbols = node.Module.Type == ModuleType.SharedLibrary,
            WorkingDirectory = generatedDir,
            EnableIncrementalLinking = config is BuildConfiguration.Debug or BuildConfiguration.Development,
            ProgramDatabaseFile = config is BuildConfiguration.Debug or BuildConfiguration.Development
                ? layout.GetModuleProgramDatabasePath(node.Name) : null,
            IncrementalLinkDatabaseFile = config is BuildConfiguration.Debug or BuildConfiguration.Development
                && linkTargetType != LinkTargetType.StaticLibrary
                ? layout.GetModuleIncrementalLinkPath(node.Name) : null,
            LibraryDirectories = new List<string>(),
            Libraries = new List<string>()
        };

        if (dependencyResult.SDKs.TryGetValue("WindowsSDK", out var windowsSdk))
        {
            foreach (var libPath in windowsSdk.LibraryPaths)
            {
                linkOptions.LibraryDirectories.Add(libPath);
                linkOptions.LibraryDirectories.Add(Path.Combine(libPath, "um"));
                linkOptions.LibraryDirectories.Add(Path.Combine(libPath, "ucrt"));
            }
            foreach (var winLib in new[] { "kernel32.lib", "user32.lib", "gdi32.lib", "ole32.lib", "shell32.lib" })
                if (!linkOptions.Libraries.Contains(winLib)) linkOptions.Libraries.Add(winLib);
        }

        linkOptions.Libraries.Add("dbghelp.lib");
        linkOptions.Libraries.Add("psapi.lib");

        if (node.Module.PlatformSettings.Windows is { } win)
        {
            linkOptions.Subsystem = win.Subsystem;
            foreach (var flag in win.LinkerFlags)
            {
                linkOptions.AdditionalFlags.Add(flag);
                // Prefer Libraries for plain .lib inputs so they participate in LIBPATH lookup
                // and are not lost if AdditionalFlags handling changes.
                if (flag.EndsWith(".lib", StringComparison.OrdinalIgnoreCase)
                    && !flag.StartsWith('/')
                    && !linkOptions.Libraries.Contains(flag))
                {
                    linkOptions.Libraries.Add(flag);
                }
            }
        }

        AddSdkLibraries(linkOptions, node, dependencyResult);
        AddThirdPartyLibraries(linkOptions, node, dependencyResult, engineDir);
        AddModuleDependencies(linkOptions, node, buildGraph, engineDir, outputLayout);
        return linkOptions;
    }

    private static void AddThirdPartyLibraries(
        LinkerOptions opts,
        BuildNode node,
        DependencyResolutionResult deps,
        string engineDir)
    {
        foreach (var thirdPartyName in node.Module.RequiredThirdParty.Concat(node.Module.OptionalThirdParty))
        {
            if (!deps.ThirdPartyLibraries.TryGetValue(thirdPartyName, out var library))
            {
                continue;
            }

            foreach (var libPath in library.LibraryPaths)
            {
                AddLibraryDirectory(opts, libPath);
            }

            AddLibraryDirectory(opts, Path.Combine(engineDir, "ThirdParty", thirdPartyName, "lib"));

            foreach (var libName in library.LibraryNames)
            {
                var libFile = libName.EndsWith(".lib", StringComparison.OrdinalIgnoreCase) ? libName : $"{libName}.lib";
                if (!opts.Libraries.Contains(libFile))
                {
                    opts.Libraries.Add(libFile);
                }
            }
        }
    }

    private static void AddLibraryDirectory(LinkerOptions opts, string libPath)
    {
        if (Directory.Exists(libPath) && !opts.LibraryDirectories.Contains(libPath))
        {
            opts.LibraryDirectories.Add(libPath);
        }
    }

    private static void AddSdkLibraries(LinkerOptions opts, BuildNode node, DependencyResolutionResult deps)
    {
        foreach (var sdkName in node.Module.RequiredSDKs.Concat(node.Module.OptionalSDKs))
        {
            if (!deps.SDKs.TryGetValue(sdkName, out var sdk) || !sdk.IsValid) continue;
            foreach (var libPath in sdk.LibraryPaths)
                if (!opts.LibraryDirectories.Contains(libPath)) opts.LibraryDirectories.Add(libPath);

            if (sdkName.Equals("SDL3", StringComparison.OrdinalIgnoreCase))
            {
                var sdlLib = sdk.LibraryPaths
                    .SelectMany(d => Directory.Exists(d) ? Directory.GetFiles(d, "SDL3*.lib") : Array.Empty<string>())
                    .FirstOrDefault(p => Path.GetFileName(p).Equals("SDL3.lib", StringComparison.OrdinalIgnoreCase)
                        || Path.GetFileName(p).Equals("SDL3-static.lib", StringComparison.OrdinalIgnoreCase));
                if (sdlLib != null)
                {
                    var lib = Path.GetFileName(sdlLib);
                    if (!opts.Libraries.Contains(lib)) opts.Libraries.Add(lib);
                    ApplyDelayLoad(opts, "SDL3.dll");
                }
            }
            else if (sdkName.Equals("VulkanSDK", StringComparison.OrdinalIgnoreCase))
            {
                var vulkan = sdk.LibraryPaths
                    .SelectMany(d => Directory.Exists(d) ? Directory.GetFiles(d, "vulkan-1.lib") : Array.Empty<string>())
                    .FirstOrDefault();
                if (vulkan != null)
                {
                    if (!opts.Libraries.Contains("vulkan-1.lib")) opts.Libraries.Add("vulkan-1.lib");
                    ApplyDelayLoad(opts, "vulkan-1.dll");
                }
            }
        }
    }

    private static void AddModuleDependencies(LinkerOptions opts, BuildNode node, DependencyGraph graph,
        string engineDir, OutputLayout outputLayout)
    {
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var depName in node.Module.PublicDependencies.Concat(node.Module.PrivateDependencies))
        {
            var depNode = graph.GetNode(depName);
            if (depNode == null) continue;

            var depLib = outputLayout.GetImportLibraryPath(depNode.Module);
            var depDir = Path.GetDirectoryName(depLib)!;
            var depFile = Path.GetFileName(depLib);

            if (seen.Add(depDir)) opts.LibraryDirectories.Add(depDir);
            if (File.Exists(depLib) && !opts.Libraries.Contains(depFile)) opts.Libraries.Add(depFile);

            if (depNode.Module.Type == ModuleType.SharedLibrary)
            {
                var linkerDir = ModuleOutputResolver.GetOutputDirectory(outputLayout.ConfigurationRoot,
                    ModuleOutputResolver.Describe(node.Module, engineDir));
                var depDir2 = ModuleOutputResolver.GetOutputDirectory(outputLayout.ConfigurationRoot,
                    ModuleOutputResolver.Describe(depNode.Module, engineDir));
                if (!linkerDir.Equals(depDir2, StringComparison.OrdinalIgnoreCase))
                    ApplyDelayLoad(opts, ModuleOutputResolver.Describe(depNode.Module, engineDir).BinaryFileName);
            }
        }
    }

    private static void ApplyDelayLoad(LinkerOptions opts, string dll)
    {
        var flag = $"/DELAYLOAD:{dll}";
        if (!opts.AdditionalFlags.Contains(flag)) opts.AdditionalFlags.Add(flag);
        if (!opts.Libraries.Contains("delayimp.lib")) opts.Libraries.Add("delayimp.lib");
    }

    private static TargetPlatform ParsePlatform(string platform) => platform.ToLowerInvariant() switch
    {
        "linux" => TargetPlatform.Linux,
        "mac" or "macos" => TargetPlatform.MacOS,
        _ => TargetPlatform.Windows
    };
}
