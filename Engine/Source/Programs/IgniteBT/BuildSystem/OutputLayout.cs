using IgniteBT.ModuleDiscovery;
using Serilog;

namespace IgniteBT.BuildSystem;

/// <summary>
/// Modular binary output layout under Output/{Platform}/{Configuration}/.
/// Bootstrap DLLs and entry-point executables live at the configuration root;
/// all other modules are placed under category/module subdirectories.
/// </summary>
public sealed class OutputLayout
{
    private static readonly HashSet<string> BootstrapModules = new(StringComparer.OrdinalIgnoreCase)
    {
        "Core",
        "CoreUObject",
        "Engine"
    };

    private static readonly Dictionary<string, string> ExecutableOutputNames = new(StringComparer.OrdinalIgnoreCase)
    {
        ["Application"] = "WindeffectsEditor.exe",
        ["Editor"] = "WindeffectsEditor.exe",
        ["CrashReporter"] = "WECrashReporter.exe",
        ["Launcher"] = "WELauncher.exe"
    };

    private readonly BuildLayout _layout;
    private readonly string _engineRoot;
    private readonly Dictionary<string, ModuleOutputDescriptor> _descriptors = new(StringComparer.OrdinalIgnoreCase);

    public OutputLayout(BuildLayout layout, string engineRoot)
    {
        _layout = layout;
        _engineRoot = engineRoot;
    }

    public string ConfigurationRoot => _layout.PlatformOutputRoot;

    public void RegisterModules(IEnumerable<DiscoveredModule> modules)
    {
        _descriptors.Clear();
        foreach (var module in modules)
        {
            _descriptors[module.Name] = Describe(module);
        }
    }

    public ModuleOutputDescriptor Describe(DiscoveredModule module)
    {
        var category = ResolveCategory(module);
        var isRoot = IsRootBinary(module);
        return new ModuleOutputDescriptor
        {
            ModuleName = module.Name,
            Category = category,
            BinaryFileName = GetBinaryFileName(module),
            IsRootBinary = isRoot,
            ModuleType = module.Type
        };
    }

    public ModuleOutputDescriptor GetDescriptor(string moduleName)
    {
        if (_descriptors.TryGetValue(moduleName, out var descriptor))
        {
            return descriptor;
        }

        throw new InvalidOperationException($"Module '{moduleName}' is not registered in the output layout.");
    }

    public string GetCategoryFolder(OutputCategory category) => category switch
    {
        OutputCategory.Runtime => "Runtime",
        OutputCategory.Editor => "Editor",
        OutputCategory.Developer => "Developer",
        OutputCategory.Programs => "Programs",
        OutputCategory.Plugins => "Plugins",
        OutputCategory.ThirdParty => "ThirdParty",
        _ => "Runtime"
    };

    public string GetModuleBinaryPath(DiscoveredModule module)
    {
        var descriptor = Describe(module);
        return Path.Combine(GetModuleOutputDirectory(descriptor), descriptor.BinaryFileName);
    }

    public string GetModuleBinaryPath(string moduleName)
    {
        var descriptor = GetDescriptor(moduleName);
        return Path.Combine(GetModuleOutputDirectory(descriptor), descriptor.BinaryFileName);
    }

    public string GetModuleOutputDirectory(ModuleOutputDescriptor descriptor)
    {
        if (descriptor.IsRootBinary)
        {
            return ConfigurationRoot;
        }

        return Path.Combine(
            ConfigurationRoot,
            GetCategoryFolder(descriptor.Category),
            descriptor.ModuleName);
    }

    public string GetImportLibraryPath(DiscoveredModule module)
    {
        var descriptor = Describe(module);
        var libName = Path.GetFileNameWithoutExtension(descriptor.BinaryFileName) + ".lib";
        return Path.Combine(GetModuleOutputDirectory(descriptor), libName);
    }

    public string GetImportLibraryPath(string moduleName)
    {
        var descriptor = GetDescriptor(moduleName);
        var libName = Path.GetFileNameWithoutExtension(descriptor.BinaryFileName) + ".lib";
        return Path.Combine(GetModuleOutputDirectory(descriptor), libName);
    }

    public string GetModuleConfigDirectory(ModuleOutputDescriptor descriptor) =>
        Path.Combine(
            ConfigurationRoot,
            "Config",
            GetCategoryFolder(descriptor.Category),
            descriptor.ModuleName);

    public void EnsureConfigurationSkeleton()
    {
        var categories = new[]
        {
            "Runtime",
            "Editor",
            "Developer",
            "Programs",
            "Plugins",
            "ThirdParty",
            "Config",
            "Content",
            "Resources"
        };

        foreach (var category in categories)
        {
            Directory.CreateDirectory(Path.Combine(ConfigurationRoot, category));
        }
    }

    public void EnsureModuleDirectories(DiscoveredModule module)
    {
        var descriptor = Describe(module);

        Directory.CreateDirectory(GetModuleOutputDirectory(descriptor));
        Directory.CreateDirectory(GetModuleConfigDirectory(descriptor));

        if (!descriptor.IsRootBinary)
        {
            Log.Debug(
                "Output layout: {Module} -> {Category}/{ModuleName}/{Binary}",
                module.Name,
                GetCategoryFolder(descriptor.Category),
                descriptor.ModuleName,
                descriptor.BinaryFileName);
        }
        else
        {
            Log.Debug("Output layout: {Module} -> {Binary} (configuration root)", module.Name, descriptor.BinaryFileName);
        }
    }

    public void PrepareModuleDirectories(IEnumerable<DiscoveredModule> modules)
    {
        EnsureConfigurationSkeleton();
        foreach (var module in modules)
        {
            EnsureModuleDirectories(module);
        }

        Log.Information("Prepared modular output layout at {Root}", ConfigurationRoot);
    }

    public void StageEngineAssets()
    {
        StageContent();
        StageConfig();
    }

    private void StageContent()
    {
        var sourceContent = Path.Combine(_engineRoot, "Content");
        var destContent = Path.Combine(ConfigurationRoot, "Content");
        if (!Directory.Exists(sourceContent))
        {
            return;
        }

        if (Directory.Exists(destContent))
        {
            return;
        }

        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(destContent)!);
            CreateDirectoryJunction(destContent, sourceContent);
            Log.Information("Linked Content: {Dest} -> {Source}", destContent, sourceContent);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to link Content directory; creating empty placeholder at {Dest}", destContent);
            Directory.CreateDirectory(destContent);
        }
    }

    private void StageConfig()
    {
        var sourceConfig = Path.Combine(_engineRoot, "Config");
        if (!Directory.Exists(sourceConfig))
        {
            return;
        }

        foreach (var descriptor in _descriptors.Values)
        {
            var moduleConfigDir = GetModuleConfigDirectory(descriptor);
            Directory.CreateDirectory(moduleConfigDir);

            var categoryFolder = GetCategoryFolder(descriptor.Category);
            var sourceModuleConfig = Path.Combine(sourceConfig, categoryFolder, descriptor.ModuleName);
            if (Directory.Exists(sourceModuleConfig))
            {
                CopyDirectory(sourceModuleConfig, moduleConfigDir);
                continue;
            }

            var sourceProgramConfig = Path.Combine(sourceConfig, descriptor.ModuleName);
            if (Directory.Exists(sourceProgramConfig))
            {
                CopyDirectory(sourceProgramConfig, moduleConfigDir);
            }
        }

        var sourceEditorConfig = Path.Combine(sourceConfig, "Editor");
        if (Directory.Exists(sourceEditorConfig))
        {
            var editorConfigRoot = Path.Combine(ConfigurationRoot, "Config", "Editor");
            Directory.CreateDirectory(editorConfigRoot);
            CopyDirectory(sourceEditorConfig, editorConfigRoot, overwrite: false);
        }
    }

    private static void CopyDirectory(string sourceDir, string destDir, bool overwrite = true)
    {
        Directory.CreateDirectory(destDir);

        foreach (var file in Directory.GetFiles(sourceDir, "*", SearchOption.AllDirectories))
        {
            var relative = Path.GetRelativePath(sourceDir, file);
            var target = Path.Combine(destDir, relative);
            Directory.CreateDirectory(Path.GetDirectoryName(target)!);

            if (!overwrite && File.Exists(target))
            {
                continue;
            }

            File.Copy(file, target, overwrite);
        }
    }

    private static void CreateDirectoryJunction(string linkPath, string targetPath)
    {
        if (OperatingSystem.IsWindows())
        {
            var psi = new System.Diagnostics.ProcessStartInfo
            {
                FileName = "cmd.exe",
                Arguments = $"/c mklink /J \"{linkPath}\" \"{targetPath}\"",
                UseShellExecute = false,
                CreateNoWindow = true
            };
            using var process = System.Diagnostics.Process.Start(psi);
            process?.WaitForExit();
            if (process is { ExitCode: not 0 } || !Directory.Exists(linkPath))
            {
                throw new InvalidOperationException($"Failed to create junction {linkPath} -> {targetPath}");
            }
            return;
        }

        if (OperatingSystem.IsLinux() || OperatingSystem.IsMacOS())
        {
            Directory.CreateSymbolicLink(linkPath, targetPath);
            return;
        }

        CopyDirectory(targetPath, linkPath);
    }

    public static string GetBinaryFileName(DiscoveredModule module)
    {
        if (module.Type == ModuleType.Executable)
        {
            if (ExecutableOutputNames.TryGetValue(module.Name, out var exeName))
            {
                return exeName;
            }

            return "WE" + module.Name + ".exe";
        }

        return "WE" + module.Name + ".dll";
    }

    public static bool IsBootstrapModule(string moduleName) => BootstrapModules.Contains(moduleName);

    public static bool IsRootBinary(DiscoveredModule module) =>
        module.Type == ModuleType.Executable || IsBootstrapModule(module.Name);

    private OutputCategory ResolveCategory(DiscoveredModule module)
    {
        var normalizedPath = module.ModuleDirectory
            .Replace('/', Path.DirectorySeparatorChar)
            .Replace('\\', Path.DirectorySeparatorChar);

        if (normalizedPath.Contains($"{Path.DirectorySeparatorChar}Plugins{Path.DirectorySeparatorChar}", StringComparison.OrdinalIgnoreCase))
        {
            return OutputCategory.Plugins;
        }

        var sourceRoot = Path.Combine(_engineRoot, "Source");
        if (!normalizedPath.StartsWith(sourceRoot, StringComparison.OrdinalIgnoreCase))
        {
            return OutputCategory.Runtime;
        }

        var relative = Path.GetRelativePath(sourceRoot, normalizedPath);
        var topLevel = relative.Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)[0];

        return topLevel.ToLowerInvariant() switch
        {
            "runtime" => OutputCategory.Runtime,
            "editor" => OutputCategory.Editor,
            "programs" => OutputCategory.Programs,
            "developer" => OutputCategory.Developer,
            _ => OutputCategory.Runtime
        };
    }
}

public enum OutputCategory
{
    Runtime,
    Editor,
    Developer,
    Programs,
    Plugins,
    ThirdParty
}

public sealed class ModuleOutputDescriptor
{
    public string ModuleName { get; init; } = string.Empty;
    public OutputCategory Category { get; init; }
    public string BinaryFileName { get; init; } = string.Empty;
    public bool IsRootBinary { get; init; }
    public ModuleType ModuleType { get; init; }
}
