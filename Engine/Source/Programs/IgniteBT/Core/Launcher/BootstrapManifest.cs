using IgniteBT.Build.Layout;
using IgniteBT.Build.Compiler;
using System.Text.Json;

namespace IgniteBT.Core.Launcher;

/// <summary>
/// Authoritative runtime registry for all engine tools. Written on every IgniteBT execution.
/// </summary>
public static class BootstrapManifest
{
    public const string SchemaKey = "schema";
    public const string WrittenAtUtcKey = "writtenAtUtc";
    public const string DefaultToolKey = "default";
    public const string KindKey = "kind";
    public const string ExecutableKey = "executable";
    public const string SourceKey = "source";

    public static bool TryLoad(string manifestPath, out BootstrapManifestData data)
    {
        data = new BootstrapManifestData();
        if (!File.Exists(manifestPath))
        {
            return false;
        }

        var document = IniFile.Parse(manifestPath);
        data.ManifestPath = manifestPath;
        document.Global.TryGetValue(SchemaKey, out var schema);
        document.Global.TryGetValue(WrittenAtUtcKey, out var writtenAtUtc);
        document.Global.TryGetValue(DefaultToolKey, out var defaultTool);

        data.Schema = schema ?? "1";
        data.WrittenAtUtc = writtenAtUtc ?? string.Empty;
        data.DefaultTool = string.IsNullOrWhiteSpace(defaultTool) ? string.Empty : defaultTool;

        foreach (var (toolName, values) in document.Sections)
        {
            values.TryGetValue(KindKey, out var kind);
            values.TryGetValue(ExecutableKey, out var executable);
            values.TryGetValue(SourceKey, out var source);

            data.Tools[toolName] = new BootstrapToolEntry
            {
                Name = toolName,
                Kind = kind ?? string.Empty,
                ExecutablePath = string.IsNullOrWhiteSpace(executable) ? string.Empty : Path.GetFullPath(executable),
                SourcePath = source ?? string.Empty
            };
        }

        return data.Tools.Count > 0;
    }

    public static bool IsToolRunnable(BootstrapToolEntry tool)
    {
        if (string.Equals(tool.Kind, "dotnet", StringComparison.OrdinalIgnoreCase))
        {
            return !string.IsNullOrWhiteSpace(tool.SourcePath) && File.Exists(tool.SourcePath);
        }

        return !string.IsNullOrWhiteSpace(tool.ExecutablePath)
            && File.Exists(tool.ExecutablePath);
    }

    public static bool IsDotNetExecutableUsable(string executablePath)
    {
        if (!File.Exists(executablePath))
        {
            return false;
        }

        var outputDirectory = Path.GetDirectoryName(executablePath);
        if (string.IsNullOrEmpty(outputDirectory))
        {
            return false;
        }

        var binaryName = Path.GetFileNameWithoutExtension(executablePath);
        return File.Exists(Path.Combine(outputDirectory, $"{binaryName}.runtimeconfig.json"))
            && File.Exists(Path.Combine(outputDirectory, $"{binaryName}.deps.json"));
    }

    public static void Refresh(EngineDescriptorData descriptor)
    {
        var document = new IniDocument
        {
            Global =
            {
                [SchemaKey] = "1",
                [WrittenAtUtcKey] = DateTime.UtcNow.ToString("O"),
                [DefaultToolKey] = descriptor.BootstrapEntry
            }
        };

        RegisterIgniteBt(document, descriptor);
        RegisterDotNetPrograms(document, descriptor);
        RegisterBuiltPrograms(document, descriptor);

        IniFile.Write(descriptor.BootstrapManifestPath, document);
    }

    private static void RegisterIgniteBt(IniDocument document, EngineDescriptorData descriptor)
    {
        var processPath = Environment.ProcessPath;
        var section = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            [KindKey] = "dotnet",
            [SourceKey] = descriptor.BootstrapEntryProjectPath
        };

        if (!string.IsNullOrWhiteSpace(processPath) && IsDotNetExecutableUsable(processPath))
        {
            section[ExecutableKey] = Path.GetFullPath(processPath);
        }

        document.Sections[descriptor.BootstrapEntry] = section;
    }

    private static void RegisterDotNetPrograms(IniDocument document, EngineDescriptorData descriptor)
    {
        if (!Directory.Exists(descriptor.ProgramsRoot))
        {
            return;
        }

        foreach (var projectPath in Directory.EnumerateFiles(descriptor.ProgramsRoot, "*.csproj", SearchOption.AllDirectories))
        {
            var toolName = Path.GetFileNameWithoutExtension(projectPath);
            if (string.IsNullOrWhiteSpace(toolName))
            {
                continue;
            }

            if (document.Sections.ContainsKey(toolName))
            {
                continue;
            }

            document.Sections[toolName] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                [KindKey] = "dotnet",
                [SourceKey] = Path.GetFullPath(projectPath)
            };
        }
    }

    private static void RegisterBuiltPrograms(IniDocument document, EngineDescriptorData descriptor)
    {
        var manifestRoot = Path.Combine(descriptor.BuildRoot, "Manifest");
        if (!Directory.Exists(manifestRoot))
        {
            return;
        }

        foreach (var platformDir in Directory.EnumerateDirectories(manifestRoot))
        {
            foreach (var configurationDir in Directory.EnumerateDirectories(platformDir))
            {
                RegisterBuiltProgramsFromLayoutManifest(document, configurationDir);
            }
        }
    }

    private static void RegisterBuiltProgramsFromLayoutManifest(IniDocument document, string manifestConfigurationRoot)
    {
        var manifestPath = Path.Combine(manifestConfigurationRoot, BuildManifestFiles.OutputLayout);
        if (!File.Exists(manifestPath))
        {
            return;
        }

        OutputLayoutManifest? manifest;
        try
        {
            manifest = JsonSerializer.Deserialize<OutputLayoutManifest>(File.ReadAllText(manifestPath));
        }
        catch
        {
            return;
        }

        if (manifest?.Modules == null || string.IsNullOrWhiteSpace(manifest.ConfigurationRoot))
        {
            return;
        }

        var configurationRoot = manifest.ConfigurationRoot;

        foreach (var module in manifest.Modules)
        {
            if (!string.Equals(module.ModuleType, nameof(ModuleType.Executable), StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            if (document.Sections.ContainsKey(module.ModuleName))
            {
                continue;
            }

            var executablePath = Path.Combine(configurationRoot, module.RelativePath);
            if (!File.Exists(executablePath))
            {
                continue;
            }

            document.Sections[module.ModuleName] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                [KindKey] = "native",
                [ExecutableKey] = Path.GetFullPath(executablePath)
            };
        }
    }

    public static BootstrapToolEntry? ResolveTool(BootstrapManifestData manifest, string? requestedTool)
    {
        if (!string.IsNullOrWhiteSpace(requestedTool)
            && manifest.Tools.TryGetValue(requestedTool, out var explicitTool))
        {
            return explicitTool;
        }

        if (!string.IsNullOrWhiteSpace(manifest.DefaultTool)
            && manifest.Tools.TryGetValue(manifest.DefaultTool, out var defaultTool))
        {
            return defaultTool;
        }

        return manifest.Tools.Values.FirstOrDefault();
    }
}

public sealed class BootstrapManifestData
{
    public string ManifestPath { get; set; } = string.Empty;
    public string Schema { get; set; } = "1";
    public string WrittenAtUtc { get; set; } = string.Empty;
    public string DefaultTool { get; set; } = string.Empty;
    public Dictionary<string, BootstrapToolEntry> Tools { get; } =
        new(StringComparer.OrdinalIgnoreCase);
}

public sealed class BootstrapToolEntry
{
    public string Name { get; set; } = string.Empty;
    public string Kind { get; set; } = string.Empty;
    public string ExecutablePath { get; set; } = string.Empty;
    public string SourcePath { get; set; } = string.Empty;
}
