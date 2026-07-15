using System.Text.Json;
using IgniteBT.Build.Dependencies;
using IgniteBT.Workspace.Modules;
using IgniteBT.Workspace.SDK;
using IgniteBT.Workspace.ThirdParty;
using Serilog;

namespace IgniteBT.Build.Layout;

/// <summary>
/// Product-oriented output layout under Build/Output/{Platform}/{Configuration}/.
/// Directories are created only when required by the modules and assets in the current build.
/// </summary>
public sealed class OutputLayout
{
    private readonly BuildLayout _layout;
    private readonly string _engineRoot;
    private readonly Dictionary<string, ModuleOutputDescriptor> _descriptors = new(StringComparer.OrdinalIgnoreCase);
    private readonly HashSet<string> _requiredDirectories = new(StringComparer.OrdinalIgnoreCase);

    public OutputLayout(BuildLayout layout, string engineRoot)
    {
        _layout = layout;
        _engineRoot = engineRoot;
    }

    public string ConfigurationRoot => _layout.PlatformOutputRoot;

    public void RegisterModules(IEnumerable<DiscoveredModule> modules)
    {
        _descriptors.Clear();
        _requiredDirectories.Clear();

        foreach (var module in modules)
        {
            var descriptor = ModuleOutputResolver.Describe(module, _engineRoot);
            _descriptors[module.Name] = descriptor;
            RegisterRequiredDirectory(ModuleOutputResolver.GetOutputDirectory(ConfigurationRoot, descriptor));
        }
    }

    public ModuleOutputDescriptor GetDescriptor(string moduleName)
    {
        if (_descriptors.TryGetValue(moduleName, out var descriptor))
        {
            return descriptor;
        }

        throw new InvalidOperationException($"Module '{moduleName}' is not registered in the output layout.");
    }

    public IReadOnlyDictionary<string, ModuleOutputDescriptor> Descriptors => _descriptors;

    public string GetModuleBinaryPath(DiscoveredModule module)
    {
        var descriptor = ModuleOutputResolver.Describe(module, _engineRoot);
        return Path.Combine(
            ModuleOutputResolver.GetOutputDirectory(ConfigurationRoot, descriptor),
            descriptor.BinaryFileName);
    }

    public string GetModuleBinaryPath(string moduleName)
    {
        var descriptor = GetDescriptor(moduleName);
        return Path.Combine(
            ModuleOutputResolver.GetOutputDirectory(ConfigurationRoot, descriptor),
            descriptor.BinaryFileName);
    }

    public string GetImportLibraryPath(DiscoveredModule module)
    {
        var descriptor = ModuleOutputResolver.Describe(module, _engineRoot);
        var libName = Path.GetFileNameWithoutExtension(descriptor.BinaryFileName) + ".lib";
        return _layout.GetModuleImportLibraryPath(module.Name, libName);
    }

    public string GetImportLibraryPath(string moduleName)
    {
        var descriptor = GetDescriptor(moduleName);
        var libName = Path.GetFileNameWithoutExtension(descriptor.BinaryFileName) + ".lib";
        return _layout.GetModuleImportLibraryPath(moduleName, libName);
    }

    public void EnsureModuleDirectories(DiscoveredModule module)
    {
        var descriptor = ModuleOutputResolver.Describe(module, _engineRoot);
        var outputDir = ModuleOutputResolver.GetOutputDirectory(ConfigurationRoot, descriptor);
        Directory.CreateDirectory(outputDir);
        Directory.CreateDirectory(_layout.GetModuleImportLibraryDirectory(module.Name));

        Log.Debug(
            "Output layout: {Module} -> {RelativePath}",
            module.Name,
            descriptor.RelativeBinaryPath);
    }

    public void PrepareModuleDirectories(IEnumerable<DiscoveredModule> modules)
    {
        RegisterModules(modules);
        RemoveLegacySourceTreeLayout(modules);

        foreach (var module in modules)
        {
            EnsureModuleDirectories(module);
        }

        Log.Information("Prepared product output layout at {Root}", ConfigurationRoot);
    }

    /// <summary>
    /// Removes output artifacts from the previous source-tree layout so mixed layouts do not persist.
    /// </summary>
    private void RemoveLegacySourceTreeLayout(IEnumerable<DiscoveredModule> modules)
    {
        if (!Directory.Exists(ConfigurationRoot))
        {
            return;
        }

        foreach (var legacyFolder in LegacySourceTreeCategoryFolders)
        {
            var legacyPath = Path.Combine(ConfigurationRoot, legacyFolder);
            if (!Directory.Exists(legacyPath))
            {
                continue;
            }

            try
            {
                Directory.Delete(legacyPath, recursive: true);
                Log.Information("Removed legacy output folder: {Path}", legacyPath);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Could not remove legacy output folder: {Path}", legacyPath);
            }
        }

        var rootBinaries = new HashSet<string>(
            modules
                .Where(m => ModuleOutputResolver.PublishesToConfigurationRoot(m, _engineRoot))
                .Select(m => ModuleOutputResolver.ResolveBinaryFileName(m)),
            StringComparer.OrdinalIgnoreCase);

        foreach (var file in Directory.GetFiles(ConfigurationRoot))
        {
            var fileName = Path.GetFileName(file);
            if (rootBinaries.Contains(fileName))
            {
                continue;
            }

            if (fileName.EndsWith(".dll", StringComparison.OrdinalIgnoreCase)
                || fileName.EndsWith(".lib", StringComparison.OrdinalIgnoreCase)
                || fileName.EndsWith(".exp", StringComparison.OrdinalIgnoreCase))
            {
                try
                {
                    File.Delete(file);
                    Log.Information("Removed legacy root binary: {File}", file);
                }
                catch (Exception ex)
                {
                    Log.Warning(ex, "Could not remove legacy root binary: {File}", file);
                }
            }
        }

        foreach (var module in modules)
        {
            if (module.Type != ModuleType.SharedLibrary)
            {
                continue;
            }

            foreach (var legacyFolder in LegacySourceTreeCategoryFolders)
            {
                var legacyModuleDir = Path.Combine(ConfigurationRoot, legacyFolder, module.Name);
                if (!Directory.Exists(legacyModuleDir))
                {
                    continue;
                }

                try
                {
                    Directory.Delete(legacyModuleDir, recursive: true);
                    Log.Information("Removed legacy module folder: {Path}", legacyModuleDir);
                }
                catch (Exception ex)
                {
                    Log.Warning(ex, "Could not remove legacy module folder: {Path}", legacyModuleDir);
                }
            }

            var legacyEngineBinary = Path.Combine(ConfigurationRoot, "Runtime", "Binaries", ModuleOutputResolver.ResolveBinaryFileName(module));
            if (File.Exists(legacyEngineBinary))
            {
                try
                {
                    File.Delete(legacyEngineBinary);
                    Log.Information("Removed legacy engine binary: {File}", legacyEngineBinary);
                }
                catch (Exception ex)
                {
                    Log.Warning(ex, "Could not remove legacy engine binary: {File}", legacyEngineBinary);
                }
            }
        }
    }

    private static readonly string[] LegacySourceTreeCategoryFolders =
    {
        "Editor",
        "Developer",
        "Content",
        "Resources",
        "Runtime",
        "Settings",
        "Dependencies",
        "Assets",
        "Config"
    };

    public void StageEngineAssets(IEnumerable<DiscoveredModule> modules)
    {
        StageEngineContent();
        StageProductAssets();
        StageRuntimePayloads();
        StageConfig();
    }

    /// <summary>
    /// Keeps the configuration root limited to bootstrap binaries and launch executables.
    /// Removes linker sidecars (.lib, .exp, .ilk, .pdb) and unexpected directories.
    /// </summary>
    public void PruneConfigurationRoot(IEnumerable<DiscoveredModule> modules)
    {
        if (!Directory.Exists(ConfigurationRoot))
        {
            return;
        }

        var allowedRootFiles = new HashSet<string>(
            modules
                .Where(m => ModuleOutputResolver.PublishesToConfigurationRoot(m, _engineRoot))
                .Select(m => ModuleOutputResolver.ResolveBinaryFileName(m)),
            StringComparer.OrdinalIgnoreCase);

        foreach (var file in Directory.GetFiles(ConfigurationRoot))
        {
            var fileName = Path.GetFileName(file);
            if (allowedRootFiles.Contains(fileName))
            {
                continue;
            }

            if (IsLinkerSidecar(fileName))
            {
                TryDeletePath(file, "configuration root sidecar");
            }
        }

        var allowedRootDirectories = new HashSet<string>(AllowedProductDirectories, StringComparer.OrdinalIgnoreCase);
        foreach (var directory in Directory.GetDirectories(ConfigurationRoot))
        {
            var dirName = Path.GetFileName(directory);
            if (allowedRootDirectories.Contains(dirName))
            {
                continue;
            }

            TryDeletePath(directory, "unexpected configuration root directory");
        }
    }

    private static readonly string[] AllowedProductDirectories =
    {
        OutputDirectories.Engine.Split('/')[0],
        OutputDirectories.ProductAssets,
        OutputDirectories.Programs,
        OutputDirectories.Plugins,
        OutputDirectories.ThirdParty,
        OutputDirectories.Saved
    };

    private static bool IsLinkerSidecar(string fileName) =>
        fileName.EndsWith(".dll", StringComparison.OrdinalIgnoreCase)
        || fileName.EndsWith(".lib", StringComparison.OrdinalIgnoreCase)
        || fileName.EndsWith(".exp", StringComparison.OrdinalIgnoreCase)
        || fileName.EndsWith(".ilk", StringComparison.OrdinalIgnoreCase)
        || fileName.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase);

    private static void TryDeletePath(string path, string description)
    {
        try
        {
            if (Directory.Exists(path))
            {
                Directory.Delete(path, recursive: true);
            }
            else if (File.Exists(path))
            {
                File.Delete(path);
            }

            Log.Information("Removed {Description}: {Path}", description, path);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Could not remove {Description}: {Path}", description, path);
        }
    }

    public void StageRuntimeDependencies(
        DependencyResolutionResult dependencyResult,
        IEnumerable<DiscoveredModule> modules)
    {
        var destDir = Path.Combine(ConfigurationRoot, OutputDirectories.ThirdParty);
        if (!AnyModuleNeedsRuntimeDependencies(modules, dependencyResult))
        {
            return;
        }

        RemoveMisplacedRuntimeDependencies(destDir);

        Directory.CreateDirectory(destDir);
        RegisterRequiredDirectory(destDir);

        var copied = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var sdkNames = CollectRequiredSdks(modules);

        foreach (var sdkName in sdkNames)
        {
            if (!dependencyResult.SDKs.TryGetValue(sdkName, out var sdkInfo) || !sdkInfo.IsValid)
            {
                continue;
            }

            CopyRuntimeBinariesFromPaths(sdkInfo.BinaryPaths.Concat(sdkInfo.LibraryPaths), destDir, copied);
        }

        foreach (var thirdPartyName in CollectRequiredThirdParty(modules))
        {
            if (!dependencyResult.ThirdPartyLibraries.TryGetValue(thirdPartyName, out var library) || !library.IsValid)
            {
                continue;
            }

            CopyRuntimeBinariesFromPaths(library.BinaryPaths.Concat(library.LibraryPaths), destDir, copied);
        }

        if (copied.Count > 0)
        {
            Log.Information("Staged {Count} runtime dependencies under {Dir}", copied.Count, destDir);
        }
    }

    /// <summary>
    /// Removes runtime DLL copies that were previously mirrored outside ThirdParty/.
    /// Runtime dependencies belong only under ThirdParty/ in the folderized layout.
    /// </summary>
    private void RemoveMisplacedRuntimeDependencies(string thirdPartyDir)
    {
        if (!Directory.Exists(thirdPartyDir))
        {
            return;
        }

        foreach (var dllPath in Directory.GetFiles(thirdPartyDir, "*.dll", SearchOption.TopDirectoryOnly))
        {
            var fileName = Path.GetFileName(dllPath);
            var misplacedPaths = new[]
            {
                Path.Combine(ConfigurationRoot, fileName),
                Path.Combine(ConfigurationRoot, OutputDirectories.EngineBinaries, fileName)
            };

            foreach (var misplaced in misplacedPaths)
            {
                if (!File.Exists(misplaced))
                {
                    continue;
                }

                try
                {
                    File.Delete(misplaced);
                    Log.Information("Removed misplaced runtime dependency: {Path}", misplaced);
                }
                catch (Exception ex)
                {
                    Log.Warning(ex, "Could not remove misplaced runtime dependency: {Path}", misplaced);
                }
            }
        }
    }

    public static string? ResolveLaunchExecutable(
        string configurationRoot,
        string target,
        IReadOnlyDictionary<string, ModuleOutputDescriptor> descriptors,
        string? layoutManifestPath = null)
    {
        if (descriptors.TryGetValue(target, out var direct) && direct.ModuleType == ModuleType.Executable)
        {
            var path = Path.Combine(configurationRoot, direct.RelativeBinaryPath);
            return File.Exists(path) ? path : null;
        }

        foreach (var descriptor in descriptors.Values)
        {
            if (descriptor.ModuleType != ModuleType.Executable)
            {
                continue;
            }

            if (descriptor.ModuleName.Equals(target, StringComparison.OrdinalIgnoreCase)
                || Path.GetFileNameWithoutExtension(descriptor.BinaryFileName)
                    .Equals(target, StringComparison.OrdinalIgnoreCase))
            {
                var path = Path.Combine(configurationRoot, descriptor.RelativeBinaryPath);
                return File.Exists(path) ? path : null;
            }
        }

        if (!string.IsNullOrWhiteSpace(layoutManifestPath) && File.Exists(layoutManifestPath))
        {
            try
            {
                var json = File.ReadAllText(layoutManifestPath);
                var manifest = JsonSerializer.Deserialize<OutputLayoutManifest>(json);
                if (manifest?.Executables != null)
                {
                    foreach (var entry in manifest.Executables)
                    {
                        if (entry.Name.Equals(target, StringComparison.OrdinalIgnoreCase)
                            || entry.ModuleName.Equals(target, StringComparison.OrdinalIgnoreCase))
                        {
                            var path = Path.Combine(configurationRoot, entry.RelativePath);
                            return File.Exists(path) ? path : null;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Debug(ex, "Failed to read output layout manifest for launch resolution");
            }
        }

        var fallback = Path.Combine(configurationRoot, target.EndsWith(".exe", StringComparison.OrdinalIgnoreCase) ? target : target + ".exe");
        return File.Exists(fallback) ? fallback : null;
    }

    private void StageEngineContent()
    {
        var sourceContent = Path.Combine(_engineRoot, "Content");
        if (!Directory.Exists(sourceContent))
        {
            return;
        }

        var destContent = Path.Combine(ConfigurationRoot, OutputDirectories.EngineContent);
        RegisterRequiredDirectory(destContent);

        if (Directory.Exists(destContent))
        {
            return;
        }

        try
        {
            CreateDirectoryLink(destContent, sourceContent);
            Log.Information("Linked engine content: {Dest} -> {Source}", destContent, sourceContent);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to link engine content; copying from {Source}", sourceContent);
            CopyDirectory(sourceContent, destContent);
        }
    }

    private void StageProductAssets()
    {
        var sourceAssets = Path.Combine(_layout.ProjectRoot, OutputDirectories.ProductAssets);
        if (!Directory.Exists(sourceAssets))
        {
            return;
        }

        var destAssets = Path.Combine(ConfigurationRoot, OutputDirectories.ProductAssets);
        RegisterRequiredDirectory(destAssets);

        if (Directory.Exists(destAssets))
        {
            return;
        }

        try
        {
            CreateDirectoryLink(destAssets, sourceAssets);
            Log.Information("Linked product assets: {Dest} -> {Source}", destAssets, sourceAssets);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to link product assets; copying from {Source}", sourceAssets);
            CopyDirectory(sourceAssets, destAssets);
        }
    }

    private void StageRuntimePayloads()
    {
        StageEngineTreeLink("Resources", OutputDirectories.EngineResources);
        StageEngineTreeLink("Shaders", OutputDirectories.EngineShaders);
        StageEngineTreeLink("Data", OutputDirectories.EngineData);
    }

    private void StageConfig()
    {
        var sourceConfig = Path.Combine(_engineRoot, "Config");
        if (!Directory.Exists(sourceConfig))
        {
            return;
        }

        var destConfig = Path.Combine(ConfigurationRoot, OutputDirectories.EngineConfig);
        RegisterRequiredDirectory(destConfig);
        CopyDirectory(sourceConfig, destConfig, overwrite: true);
        Log.Information("Staged runtime config from {Source} to {Dest}", sourceConfig, destConfig);
    }

    public void RefreshLayoutManifest(IEnumerable<DiscoveredModule> modules) =>
        WriteLayoutManifest(modules);

    private void WriteLayoutManifest(IEnumerable<DiscoveredModule> modules)
    {
        var manifestDir = _layout.PlatformManifestRoot;
        Directory.CreateDirectory(manifestDir);

        var manifest = new OutputLayoutManifest
        {
            ConfigurationRoot = ConfigurationRoot,
            GeneratedAtUtc = DateTime.UtcNow.ToString("O")
        };

        foreach (var module in modules)
        {
            var descriptor = ModuleOutputResolver.Describe(module, _engineRoot);
            manifest.Modules.Add(new OutputLayoutModuleEntry
            {
                ModuleName = descriptor.ModuleName,
                BinaryFileName = descriptor.BinaryFileName,
                RelativePath = descriptor.RelativeBinaryPath,
                Placement = descriptor.Placement.ToString(),
                ModuleType = descriptor.ModuleType.ToString(),
                IsBootstrapBinary = descriptor.IsBootstrapBinary
            });

            if (descriptor.ModuleType == ModuleType.Executable
                && descriptor.Placement == OutputPlacement.ConfigurationRoot)
            {
                manifest.Executables.Add(new OutputLayoutExecutableEntry
                {
                    ModuleName = descriptor.ModuleName,
                    Name = Path.GetFileNameWithoutExtension(descriptor.BinaryFileName),
                    RelativePath = descriptor.RelativeBinaryPath
                });
            }

            if (descriptor.IsBootstrapBinary)
            {
                manifest.BootstrapBinaries.Add(new OutputLayoutBootstrapEntry
                {
                    ModuleName = descriptor.ModuleName,
                    BinaryFileName = descriptor.BinaryFileName,
                    RelativePath = descriptor.RelativeBinaryPath
                });
            }
        }

        var manifestPath = _layout.GetOutputLayoutManifestPath();
        var json = JsonSerializer.Serialize(manifest, new JsonSerializerOptions { WriteIndented = true });
        File.WriteAllText(manifestPath, json);
        Log.Information("Wrote build layout manifest to {Path}", manifestPath);
    }

    private void StageEngineTreeLink(string sourceFolderName, string destinationRelativePath)
    {
        var sourcePath = Path.Combine(_engineRoot, sourceFolderName);
        if (!Directory.Exists(sourcePath))
        {
            return;
        }

        var destPath = Path.Combine(ConfigurationRoot, destinationRelativePath);
        RegisterRequiredDirectory(destPath);

        if (Directory.Exists(destPath))
        {
            return;
        }

        try
        {
            CreateDirectoryLink(destPath, sourcePath);
            Log.Information("Linked runtime payload: {Dest} -> {Source}", destPath, sourcePath);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to link {Dest}; copying from {Source}", destPath, sourcePath);
            CopyDirectory(sourcePath, destPath);
        }
    }

    private void RegisterRequiredDirectory(string path)
    {
        _requiredDirectories.Add(path);
        var parent = Path.GetDirectoryName(path);
        while (!string.IsNullOrEmpty(parent)
            && parent.StartsWith(ConfigurationRoot, StringComparison.OrdinalIgnoreCase))
        {
            _requiredDirectories.Add(parent);
            parent = Path.GetDirectoryName(parent);
        }
    }

    private static bool AnyModuleNeedsRuntimeDependencies(
        IEnumerable<DiscoveredModule> modules,
        DependencyResolutionResult dependencyResult)
    {
        return CollectRequiredSdks(modules).Any(name =>
            dependencyResult.SDKs.TryGetValue(name, out var sdk) && sdk.IsValid
            && (sdk.BinaryPaths.Count > 0 || sdk.LibraryPaths.Count > 0))
            || CollectRequiredThirdParty(modules).Any(name =>
                dependencyResult.ThirdPartyLibraries.TryGetValue(name, out var lib) && lib.IsValid
                && (lib.BinaryPaths.Count > 0 || lib.LibraryPaths.Count > 0));
    }

    private static HashSet<string> CollectRequiredSdks(IEnumerable<DiscoveredModule> modules)
    {
        var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var module in modules)
        {
            foreach (var sdk in module.RequiredSDKs.Concat(module.OptionalSDKs))
            {
                names.Add(sdk);
            }
        }

        return names;
    }

    private static HashSet<string> CollectRequiredThirdParty(IEnumerable<DiscoveredModule> modules)
    {
        var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var module in modules)
        {
            foreach (var library in module.RequiredThirdParty.Concat(module.OptionalThirdParty))
            {
                names.Add(library);
            }
        }

        return names;
    }

    private static void CopyRuntimeBinariesFromPaths(IEnumerable<string> searchPaths, string destDir, HashSet<string> copied)
    {
        foreach (var searchPath in searchPaths)
        {
            if (!Directory.Exists(searchPath))
            {
                continue;
            }

            foreach (var dll in Directory.GetFiles(searchPath, "*.dll", SearchOption.TopDirectoryOnly))
            {
                var fileName = Path.GetFileName(dll);
                if (!copied.Add(fileName))
                {
                    continue;
                }

                var target = Path.Combine(destDir, fileName);
                File.Copy(dll, target, overwrite: true);
                Log.Debug("Staged dependency: {File}", target);
            }
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

    private static void CreateDirectoryLink(string linkPath, string targetPath)
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
}

public sealed class ModuleOutputDescriptor
{
    public string ModuleName { get; init; } = string.Empty;
    public OutputPlacement Placement { get; init; }
    public string BinaryFileName { get; init; } = string.Empty;
    public string RelativeBinaryPath { get; init; } = string.Empty;
    public ModuleType ModuleType { get; init; }
    public bool IsBootstrapBinary { get; init; }
}

public sealed class OutputLayoutManifest
{
    public string ConfigurationRoot { get; set; } = string.Empty;
    public string GeneratedAtUtc { get; set; } = string.Empty;
    public List<OutputLayoutModuleEntry> Modules { get; set; } = new();
    public List<OutputLayoutExecutableEntry> Executables { get; set; } = new();
    public List<OutputLayoutBootstrapEntry> BootstrapBinaries { get; set; } = new();
}

public sealed class OutputLayoutModuleEntry
{
    public string ModuleName { get; set; } = string.Empty;
    public string BinaryFileName { get; set; } = string.Empty;
    public string RelativePath { get; set; } = string.Empty;
    public string Placement { get; set; } = string.Empty;
    public string ModuleType { get; set; } = string.Empty;
    public bool IsBootstrapBinary { get; set; }
}

public sealed class OutputLayoutExecutableEntry
{
    public string ModuleName { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string RelativePath { get; set; } = string.Empty;
}

public sealed class OutputLayoutBootstrapEntry
{
    public string ModuleName { get; set; } = string.Empty;
    public string BinaryFileName { get; set; } = string.Empty;
    public string RelativePath { get; set; } = string.Empty;
}
