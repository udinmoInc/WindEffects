using System.IO.Compression;
using System.Text.Json;
using IgniteBT.Build.Layout;
using IgniteBT.Workspace.Modules;
using Serilog;

namespace IgniteBT.CLI;

/// <summary>
/// Packages a built product layout into a distributable zip under Build/Packages.
/// </summary>
public static class PackageCommand
{
    public static async Task<int> Execute(string[] args)
    {
        var parsed = CommandSchemas.Package.Parse(args);
        if (!CommandLineHelpers.TryReportErrors(parsed))
        {
            return 1;
        }

        var target = parsed.ResolveTarget("Editor");
        var config = parsed.GetOption("config", "Shipping");
        var platform = CommandLineHelpers.NormalizePlatform(parsed.GetOption("platform", string.Empty));
        var skipBuild = parsed.HasFlag("skip-build");

        Log.Information("Package Command");
        Log.Information("Target: {Target}", target);
        Log.Information("Configuration: {Config}", config);
        Log.Information("Platform: {Platform}", platform);

        try
        {
            var currentDir = Directory.GetCurrentDirectory();
            var projectRoot = BuildLayout.FindProjectRoot(currentDir);
            var engineDir = BuildLayout.FindEngineRoot(currentDir);

            if (string.IsNullOrEmpty(projectRoot) || string.IsNullOrEmpty(engineDir) || !Directory.Exists(engineDir))
            {
                Log.Error("Could not find project root from {Dir}", currentDir);
                return 1;
            }

            var buildConfig = CommandLineHelpers.ParseConfiguration(config);
            var layout = BuildLayout.Resolve(currentDir, platform, buildConfig);
            var outputRoot = layout.PlatformOutputRoot;

            if (!skipBuild)
            {
                Log.Information("Building {Target} ({Config}) before packaging...", target, config);
                var buildArgs = new List<string> { "--target", target, "--config", config, "--platform", platform };
                var jobs = parsed.GetOption("jobs", string.Empty);
                if (!string.IsNullOrWhiteSpace(jobs))
                {
                    buildArgs.Add("--jobs");
                    buildArgs.Add(jobs);
                }

                var buildResult = await BuildCommand.Execute(buildArgs.ToArray());
                if (buildResult != 0)
                {
                    Log.Error("Build failed; packaging aborted.");
                    return buildResult;
                }
            }

            if (!Directory.Exists(outputRoot))
            {
                Log.Error("Build output not found at {OutputRoot}. Run 'we build' first or omit --skip-build.", outputRoot);
                return 1;
            }

            var discovery = new ModuleDiscoverer(engineDir, config, platform);
            var modules = await discovery.DiscoverModulesAsync();
            var outputLayout = new OutputLayout(layout, engineDir);
            outputLayout.RegisterModules(modules);

            var normalizedTarget = NormalizePackageTarget(target);
            var executablePath = OutputLayout.ResolveLaunchExecutable(
                outputRoot,
                normalizedTarget,
                outputLayout.Descriptors,
                layout.GetOutputLayoutManifestPath());

            if (string.IsNullOrEmpty(executablePath) || !File.Exists(executablePath))
            {
                Log.Error(
                    "Executable for target {Target} not found under {OutputRoot}. Build the target before packaging.",
                    normalizedTarget,
                    outputRoot);
                return 1;
            }

            var packagesRoot = Path.Combine(layout.BuildRoot, "Packages");
            Directory.CreateDirectory(packagesRoot);

            var stamp = DateTime.UtcNow.ToString("yyyyMMdd-HHmmss");
            var packageName = $"WindEffects-{normalizedTarget}-{layout.PlatformFolder}-{layout.ConfigurationFolder}-{stamp}";
            var stagingDir = Path.Combine(layout.TempRoot, "PackageStaging", packageName);
            var zipPath = Path.Combine(packagesRoot, packageName + ".zip");

            if (Directory.Exists(stagingDir))
            {
                Directory.Delete(stagingDir, recursive: true);
            }

            Directory.CreateDirectory(stagingDir);

            Log.Information("Staging distributable from {OutputRoot}", outputRoot);
            CopyDistributableTree(outputRoot, stagingDir);

            WritePackageManifest(stagingDir, new PackageManifest
            {
                Target = normalizedTarget,
                Configuration = layout.ConfigurationFolder,
                Platform = layout.PlatformFolder,
                Executable = Path.GetRelativePath(outputRoot, executablePath).Replace('\\', '/'),
                CreatedUtc = DateTime.UtcNow.ToString("o"),
                SourceOutput = outputRoot
            });

            if (File.Exists(zipPath))
            {
                File.Delete(zipPath);
            }

            Log.Information("Creating package archive {Zip}", zipPath);
            ZipFile.CreateFromDirectory(stagingDir, zipPath, CompressionLevel.Optimal, includeBaseDirectory: false);

            try
            {
                Directory.Delete(stagingDir, recursive: true);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Failed to clean package staging directory {Staging}", stagingDir);
            }

            var zipInfo = new FileInfo(zipPath);
            Log.Information(
                "Packaged {Target} -> {Zip} ({SizeMb:F1} MB)",
                normalizedTarget,
                zipPath,
                zipInfo.Length / (1024.0 * 1024.0));

            return 0;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Package failed");
            return 1;
        }
    }

    private static string NormalizePackageTarget(string target) =>
        target.ToLowerInvariant() switch
        {
            "editor" => "Editor",
            "welauncher" or "launcher" => "WeLauncher",
            "we" or "cli" => "We",
            "crashreporter" or "crash" => "CrashReporter",
            _ => target
        };

    private static void CopyDistributableTree(string sourceRoot, string destRoot)
    {
        foreach (var dir in Directory.EnumerateDirectories(sourceRoot, "*", SearchOption.AllDirectories))
        {
            var relative = Path.GetRelativePath(sourceRoot, dir);
            Directory.CreateDirectory(Path.Combine(destRoot, relative));
        }

        foreach (var file in Directory.EnumerateFiles(sourceRoot, "*", SearchOption.AllDirectories))
        {
            // Skip bulky debug symbols from distributable packages.
            var ext = Path.GetExtension(file);
            if (ext.Equals(".pdb", StringComparison.OrdinalIgnoreCase) ||
                ext.Equals(".ilk", StringComparison.OrdinalIgnoreCase) ||
                ext.Equals(".exp", StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            var relative = Path.GetRelativePath(sourceRoot, file);
            var dest = Path.Combine(destRoot, relative);
            Directory.CreateDirectory(Path.GetDirectoryName(dest)!);

            // Materialize junctions/symlinks (e.g. Assets) as real copies for portable packages.
            File.Copy(file, dest, overwrite: true);
        }
    }

    private static void WritePackageManifest(string stagingDir, PackageManifest manifest)
    {
        var path = Path.Combine(stagingDir, "package.json");
        var json = JsonSerializer.Serialize(manifest, new JsonSerializerOptions { WriteIndented = true });
        File.WriteAllText(path, json);
    }

    private sealed class PackageManifest
    {
        public string Target { get; set; } = "";
        public string Configuration { get; set; } = "";
        public string Platform { get; set; } = "";
        public string Executable { get; set; } = "";
        public string CreatedUtc { get; set; } = "";
        public string SourceOutput { get; set; } = "";
    }
}
