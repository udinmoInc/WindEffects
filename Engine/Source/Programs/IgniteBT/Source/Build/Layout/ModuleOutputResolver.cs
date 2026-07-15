using IgniteBT.Workspace.Modules;

namespace IgniteBT.Build.Layout;

/// <summary>
/// Derives output placement and binary names from module metadata and source location.
/// </summary>
public static class ModuleOutputResolver
{
    public static ModuleOutputDescriptor Describe(DiscoveredModule module, string engineRoot)
    {
        var placement = module.OutputPlacement != OutputPlacement.Unspecified
            ? module.OutputPlacement
            : InferPlacement(module, engineRoot);

        var binaryFileName = ResolveBinaryFileName(module);
        var isBootstrap = module.IsBootstrapBinary
            || placement == OutputPlacement.ConfigurationRoot && module.Type == ModuleType.SharedLibrary;

        return new ModuleOutputDescriptor
        {
            ModuleName = module.Name,
            Placement = placement,
            BinaryFileName = binaryFileName,
            ModuleType = module.Type,
            IsBootstrapBinary = isBootstrap,
            RelativeBinaryPath = GetRelativeBinaryPath(placement, module.Name, binaryFileName)
        };
    }

    public static OutputPlacement InferPlacement(DiscoveredModule module, string engineRoot)
    {
        if (module.IsPlugin)
        {
            return OutputPlacement.Plugin;
        }

        if (module.IsBootstrapBinary)
        {
            return OutputPlacement.ConfigurationRoot;
        }

        if (module.Type == ModuleType.Executable)
        {
            return IsToolProgram(module, engineRoot)
                ? OutputPlacement.Program
                : OutputPlacement.ConfigurationRoot;
        }

        return OutputPlacement.EngineBinary;
    }

    public static string ResolveBinaryFileName(DiscoveredModule module)
    {
        if (!string.IsNullOrWhiteSpace(module.BinaryName))
        {
            return module.BinaryName;
        }

        var extension = module.Type == ModuleType.Executable ? ".exe" : ".dll";
        return "WE" + module.Name + extension;
    }

    public static string GetRelativeBinaryPath(OutputPlacement placement, string moduleName, string binaryFileName) =>
        placement switch
        {
            OutputPlacement.ConfigurationRoot => binaryFileName,
            OutputPlacement.EngineBinary => Path.Combine(OutputDirectories.EngineBinaries, binaryFileName),
            OutputPlacement.Program => Path.Combine(OutputDirectories.Programs, moduleName, binaryFileName),
            OutputPlacement.Plugin => Path.Combine(OutputDirectories.Plugins, moduleName, binaryFileName),
            _ => binaryFileName
        };

    public static string GetOutputDirectory(string configurationRoot, ModuleOutputDescriptor descriptor) =>
        descriptor.Placement switch
        {
            OutputPlacement.ConfigurationRoot => configurationRoot,
            OutputPlacement.EngineBinary => Path.Combine(configurationRoot, OutputDirectories.EngineBinaries),
            OutputPlacement.Program => Path.Combine(configurationRoot, OutputDirectories.Programs, descriptor.ModuleName),
            OutputPlacement.Plugin => Path.Combine(configurationRoot, OutputDirectories.Plugins, descriptor.ModuleName),
            _ => configurationRoot
        };

    public static bool PublishesToConfigurationRoot(DiscoveredModule module, string engineRoot)
    {
        var placement = module.OutputPlacement != OutputPlacement.Unspecified
            ? module.OutputPlacement
            : InferPlacement(module, engineRoot);

        return placement == OutputPlacement.ConfigurationRoot;
    }

    private static bool IsToolProgram(DiscoveredModule module, string engineRoot)
    {
        var sourceRoot = Path.Combine(engineRoot, "Source");
        if (!module.ModuleDirectory.StartsWith(sourceRoot, StringComparison.OrdinalIgnoreCase))
        {
            return false;
        }

        var relative = Path.GetRelativePath(sourceRoot, module.ModuleDirectory)
            .Replace('/', Path.DirectorySeparatorChar)
            .Replace('\\', Path.DirectorySeparatorChar);

        var segments = relative.Split(Path.DirectorySeparatorChar, StringSplitOptions.RemoveEmptyEntries);
        return segments.Length > 0
            && segments[0].Equals("Programs", StringComparison.OrdinalIgnoreCase);
    }
}
