using IgniteBT.Build.Layout;
using IgniteBT.Build.Compiler;

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
        var outputRoot = Path.Combine(descriptor.BuildRoot, "Output");
        if (!Directory.Exists(outputRoot))
        {
            return;
        }

        foreach (var executablePath in Directory.EnumerateFiles(outputRoot, "*.exe", SearchOption.AllDirectories))
        {
            var toolName = MapExecutableToToolName(Path.GetFileName(executablePath));
            if (string.IsNullOrWhiteSpace(toolName) || document.Sections.ContainsKey(toolName))
            {
                continue;
            }

            document.Sections[toolName] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                [KindKey] = "native",
                [ExecutableKey] = Path.GetFullPath(executablePath)
            };
        }
    }

    private static string MapExecutableToToolName(string fileName)
    {
        var name = Path.GetFileNameWithoutExtension(fileName);
        if (string.IsNullOrWhiteSpace(name))
        {
            return string.Empty;
        }

        if (name.StartsWith("WE", StringComparison.OrdinalIgnoreCase))
        {
            name = name[2..];
        }

        return name switch
        {
            "WindeffectsEditor" => "Editor",
            _ => name
        };
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
