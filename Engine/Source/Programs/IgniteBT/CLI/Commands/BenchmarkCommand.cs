using Serilog;
using IgniteBT.Benchmarks;
using IgniteBT.Build.Compiler;
using IgniteBT.Build.Layout;

namespace IgniteBT.CLI;

public static class BenchmarkCommand
{
    public static async Task<int> Execute(string[] args)
    {
        var projectRoot = Directory.GetCurrentDirectory();
        var exe = Path.Combine(projectRoot, "Build", "Intermediate", "IgniteBT", "Development", "net8.0", "IgniteBT.exe");
        if (!File.Exists(exe))
            exe = "dotnet";

        var layout = BuildLayout.Resolve(projectRoot, "Win64", BuildConfiguration.Development);
        var outputDir = Path.Combine(layout.BuildRoot, "Benchmarks");
        var jobCounts = ResolveJobCounts(args);

        Log.Information("Running IgniteBT v{Version} benchmarks from {Root}", BuildBenchmark.Version, projectRoot);
        if (jobCounts.Length > 0)
            Log.Information("Job counts: {Jobs}", string.Join(", ", jobCounts));

        var report = await BuildBenchmark.RunAllAsync(exe, projectRoot, jobCounts);
        BuildBenchmark.WriteReports(report, outputDir);

        Console.WriteLine();
        Console.WriteLine($"IgniteBT Benchmark Report (v{BuildBenchmark.Version})");
        Console.WriteLine(new string('-', 60));
        Console.WriteLine($"{"Category",-14} {"Scenario",-14} {"Time (ms)",10} {"Exit",5}");
        Console.WriteLine(new string('-', 60));
        foreach (var r in report.Results)
            Console.WriteLine($"{r.Category,-14} {r.Scenario,-14} {r.ElapsedMs,10} {r.ExitCode,5}");

        var failed = report.Results.Count(r => r.ExitCode != 0);
        if (failed > 0)
            Log.Warning("Benchmark: {Failed}/{Total} scenarios failed", failed, report.Results.Count);
        else
            Log.Information("Benchmark: all {Total} scenarios passed", report.Results.Count);

        return failed == 0 ? 0 : 1;
    }

    private static int[] ResolveJobCounts(string[] args)
    {
        for (var i = 0; i < args.Length - 1; i++)
        {
            if (args[i] is "--jobs" or "-j")
            {
                return args[i + 1]
                    .Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
                    .Select(s => int.Parse(s))
                    .ToArray();
            }
        }

        var cores = Environment.ProcessorCount;
        return new[] { Math.Max(1, cores / 2), cores, Math.Min(cores * 2, 64) }.Distinct().ToArray();
    }
}
