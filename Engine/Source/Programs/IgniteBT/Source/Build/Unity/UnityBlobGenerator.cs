using Serilog;

namespace IgniteBT.Build.Unity;

/// <summary>
/// Generates temporary unity .cpp files from planned unity blobs.
/// </summary>
public static class UnityBlobGenerator
{
    public static string Generate(UnityBlob blob, string unityDirectory, int blobIndex)
    {
        if (blob.SourceFiles.Count <= 1)
            return blob.SourceFiles[0];

        Directory.CreateDirectory(unityDirectory);
        var unityFileName = $"Unity_{blobIndex}_{Path.GetFileNameWithoutExtension(blob.SourceFiles[0])}.gen.cpp";
        var unityPath = Path.Combine(unityDirectory, unityFileName);

        if (File.Exists(unityPath))
        {
            var existingSources = ReadEmbeddedSources(unityPath);
            if (existingSources.SequenceEqual(blob.SourceFiles, StringComparer.OrdinalIgnoreCase))
                return unityPath;
        }

        var sb = new System.Text.StringBuilder();
        sb.AppendLine("// IgniteBT Unity Build — auto-generated, do not edit");
        sb.AppendLine($"// Sources: {blob.SourceFiles.Count}");
        sb.AppendLine("// IBT_SOURCES_BEGIN");
        foreach (var src in blob.SourceFiles)
            sb.AppendLine($"// IBT_SOURCE: {src}");
        sb.AppendLine("// IBT_SOURCES_END");
        sb.AppendLine();

        foreach (var src in blob.SourceFiles)
        {
            sb.AppendLine($"#include \"{src.Replace('\\', '/')}\"");
        }

        File.WriteAllText(unityPath, sb.ToString());
        Log.Debug("Generated unity blob {Path} ({Count} sources)", unityPath, blob.SourceFiles.Count);
        return unityPath;
    }

    private static List<string> ReadEmbeddedSources(string unityPath)
    {
        var sources = new List<string>();
        foreach (var line in File.ReadLines(unityPath))
        {
            if (line.StartsWith("// IBT_SOURCE: ", StringComparison.Ordinal))
                sources.Add(line["// IBT_SOURCE: ".Length..].Trim());
        }
        return sources;
    }
}
