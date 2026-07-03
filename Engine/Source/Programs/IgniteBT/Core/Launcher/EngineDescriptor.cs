namespace IgniteBT.Core.Launcher;

/// <summary>
/// Stable bootstrap protocol between the <c>we</c> launcher and the engine installation.
/// Only the descriptor filename and key names are launcher surface area.
/// </summary>
public static class EngineDescriptor
{
    public const string FileName = "WindEffects.engine";

    public const string SchemaKey = "schema";
    public const string EngineVersionKey = "engine.version";
    public const string ProgramsRootKey = "ProgramsRoot";
    public const string BuildRootKey = "BuildRoot";
    public const string AssetsRootKey = "AssetsRoot";
    public const string ProjectsRootKey = "ProjectsRoot";
    public const string BootstrapManifestKey = "bootstrap.manifest";
    public const string BootstrapEntryKey = "bootstrap.entry";
    public const string BootstrapEntrySourceKey = "bootstrap.entry.source";

    public static bool TryFindEngineRoot(string startDirectory, out string engineRoot)
    {
        engineRoot = string.Empty;

        if (!string.IsNullOrWhiteSpace(startDirectory) && TryFindFromDirectory(startDirectory, out engineRoot))
        {
            return true;
        }

        var cwd = Directory.GetCurrentDirectory();
        if (!string.Equals(cwd, startDirectory, StringComparison.OrdinalIgnoreCase)
            && TryFindFromDirectory(cwd, out engineRoot))
        {
            return true;
        }

        var envRoot = Environment.GetEnvironmentVariable("WE_PROJECT_ROOT")
            ?? Environment.GetEnvironmentVariable("WE_ENGINE_ROOT");
        if (!string.IsNullOrWhiteSpace(envRoot))
        {
            var candidate = envRoot;
            if (File.Exists(Path.Combine(candidate, FileName)))
            {
                engineRoot = Path.GetFullPath(candidate);
                return true;
            }

            var parent = Path.GetDirectoryName(candidate.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar));
            if (!string.IsNullOrWhiteSpace(parent) && File.Exists(Path.Combine(parent, FileName)))
            {
                engineRoot = Path.GetFullPath(parent);
                return true;
            }
        }

        return false;
    }

    public static bool TryLoad(string engineRoot, out EngineDescriptorData data)
    {
        data = new EngineDescriptorData();
        var descriptorPath = Path.Combine(engineRoot, FileName);
        if (!File.Exists(descriptorPath))
        {
            return false;
        }

        var values = IniFile.Parse(descriptorPath);
        if (!values.Global.TryGetValue(BootstrapManifestKey, out var manifestRelative)
            || string.IsNullOrWhiteSpace(manifestRelative))
        {
            return false;
        }

        if (!values.Global.TryGetValue(BootstrapEntryKey, out var bootstrapEntry)
            || string.IsNullOrWhiteSpace(bootstrapEntry))
        {
            return false;
        }

        values.Global.TryGetValue(SchemaKey, out var schema);
        values.Global.TryGetValue(EngineVersionKey, out var engineVersion);
        values.Global.TryGetValue(ProgramsRootKey, out var programsRoot);
        values.Global.TryGetValue(BuildRootKey, out var buildRoot);
        values.Global.TryGetValue(AssetsRootKey, out var assetsRoot);
        values.Global.TryGetValue(ProjectsRootKey, out var projectsRoot);
        values.Global.TryGetValue(BootstrapEntrySourceKey, out var bootstrapEntrySource);

        data.EngineRoot = Path.GetFullPath(engineRoot);
        data.DescriptorPath = descriptorPath;
        data.Schema = schema ?? "1";
        data.EngineVersion = engineVersion ?? string.Empty;
        data.ProgramsRoot = ResolveRelative(data.EngineRoot, programsRoot ?? "Engine/Source/Programs");
        data.BuildRoot = ResolveRelative(data.EngineRoot, buildRoot ?? "Build");
        data.AssetsRoot = ResolveRelative(data.EngineRoot, assetsRoot ?? "Assets");
        data.ProjectsRoot = ResolveRelative(data.EngineRoot, projectsRoot ?? "Projects");
        data.BootstrapManifestPath = ResolveRelative(data.EngineRoot, manifestRelative);
        data.BootstrapEntry = bootstrapEntry;
        data.BootstrapEntrySource = bootstrapEntrySource ?? string.Empty;
        data.BootstrapEntryProjectPath = string.IsNullOrWhiteSpace(bootstrapEntrySource)
            ? string.Empty
            : ResolveRelative(data.ProgramsRoot, bootstrapEntrySource);
        return true;
    }

    private static bool TryFindFromDirectory(string startDirectory, out string engineRoot)
    {
        engineRoot = string.Empty;
        var dir = new DirectoryInfo(Path.GetFullPath(startDirectory));

        while (dir != null)
        {
            if (File.Exists(Path.Combine(dir.FullName, FileName)))
            {
                engineRoot = dir.FullName;
                return true;
            }

            dir = dir.Parent;
        }

        return false;
    }

    private static string ResolveRelative(string root, string relativePath) =>
        Path.GetFullPath(Path.Combine(root, relativePath.Replace('/', Path.DirectorySeparatorChar)));
}

public sealed class EngineDescriptorData
{
    public string EngineRoot { get; set; } = string.Empty;
    public string DescriptorPath { get; set; } = string.Empty;
    public string Schema { get; set; } = "1";
    public string EngineVersion { get; set; } = string.Empty;
    public string ProgramsRoot { get; set; } = string.Empty;
    public string BuildRoot { get; set; } = string.Empty;
    public string AssetsRoot { get; set; } = string.Empty;
    public string ProjectsRoot { get; set; } = string.Empty;
    public string BootstrapManifestPath { get; set; } = string.Empty;
    public string BootstrapEntry { get; set; } = string.Empty;
    public string BootstrapEntrySource { get; set; } = string.Empty;
    public string BootstrapEntryProjectPath { get; set; } = string.Empty;
}
