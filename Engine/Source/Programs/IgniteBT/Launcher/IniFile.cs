namespace IgniteBT.Launcher;

public static class IniFile
{
    public static IniDocument Parse(string path)
    {
        var document = new IniDocument();
        var section = string.Empty;

        foreach (var rawLine in File.ReadAllLines(path))
        {
            var line = rawLine.Trim();
            if (line.Length == 0 || line.StartsWith('#'))
            {
                continue;
            }

            if (line.StartsWith('[') && line.EndsWith(']') && line.Length > 2)
            {
                section = line[1..^1].Trim();
                continue;
            }

            var separator = line.IndexOf('=');
            if (separator <= 0)
            {
                continue;
            }

            var key = line[..separator].Trim();
            var value = line[(separator + 1)..].Trim();

            if (string.IsNullOrEmpty(section))
            {
                document.Global[key] = value;
            }
            else if (!document.Sections.TryGetValue(section, out var sectionValues))
            {
                sectionValues = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                document.Sections[section] = sectionValues;
                sectionValues[key] = value;
            }
            else
            {
                sectionValues[key] = value;
            }
        }

        return document;
    }

    public static void Write(string path, IniDocument document)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        using var writer = new StreamWriter(path);

        WriteSection(writer, null, document.Global);
        foreach (var (sectionName, values) in document.Sections.OrderBy(static pair => pair.Key, StringComparer.OrdinalIgnoreCase))
        {
            WriteSection(writer, sectionName, values);
        }
    }

    private static void WriteSection(TextWriter writer, string? sectionName, Dictionary<string, string> values)
    {
        if (values.Count == 0)
        {
            return;
        }

        if (!string.IsNullOrEmpty(sectionName))
        {
            writer.WriteLine();
            writer.WriteLine($"[{sectionName}]");
        }

        foreach (var (key, value) in values.OrderBy(static pair => pair.Key, StringComparer.OrdinalIgnoreCase))
        {
            writer.WriteLine($"{key}={value}");
        }
    }
}

public sealed class IniDocument
{
    public Dictionary<string, string> Global { get; } = new(StringComparer.OrdinalIgnoreCase);
    public Dictionary<string, Dictionary<string, string>> Sections { get; } =
        new(StringComparer.OrdinalIgnoreCase);
}
