using System.Diagnostics;
using System.Globalization;
using System.Text;
using System.Text.Json;
using Serilog;

namespace IgniteBT.Benchmarks;

/// <summary>
/// Automated build benchmark suite with multi-format reporting.
/// </summary>
public static class BuildBenchmark
{
    public const string Version = "1.0.0";

    public sealed class BenchmarkResult
    {
        public string Scenario { get; set; } = string.Empty;
        public string Category { get; set; } = string.Empty;
        public long ElapsedMs { get; set; }
        public int ExitCode { get; set; }
        public int Jobs { get; set; }
        public bool Unity { get; set; }
        public string Config { get; set; } = "Development";
        public DateTimeOffset Timestamp { get; set; } = DateTimeOffset.UtcNow;
    }

    public sealed class BenchmarkReport
    {
        public string IgniteBtVersion { get; set; } = Version;
        public string ProjectRoot { get; set; } = string.Empty;
        public string Executable { get; set; } = string.Empty;
        public int ProcessorCount { get; set; }
        public DateTimeOffset StartedAt { get; set; }
        public DateTimeOffset CompletedAt { get; set; }
        public List<BenchmarkResult> Results { get; set; } = new();
    }

    public static async Task<BenchmarkReport> RunAllAsync(string weExecutable, string projectRoot, int[]? jobCounts = null)
    {
        jobCounts ??= [Environment.ProcessorCount];
        var report = new BenchmarkReport
        {
            ProjectRoot = projectRoot,
            Executable = weExecutable,
            ProcessorCount = Environment.ProcessorCount,
            StartedAt = DateTimeOffset.UtcNow
        };

        var scenarios = new List<(string Category, string Name, string[] Args)>
        {
            ("lifecycle", "cold", ["rebuild", "--config", "Development"]),
            ("lifecycle", "warm", ["build", "--config", "Development"]),
            ("lifecycle", "noop", ["build", "--config", "Development"]),
            ("lifecycle", "noop_repeat", ["build", "--config", "Development"]),
            ("target", "editor", ["build", "--config", "Development", "--target", "Editor"]),
            ("unity", "unity_on", ["build", "--config", "Development", "--unity"]),
            ("unity", "unity_off", ["build", "--config", "Development"]),
        };

        foreach (var jobs in jobCounts.Distinct().OrderBy(j => j))
        {
            scenarios.Add(("parallelism", $"jobs_{jobs}", ["build", "--config", "Development", "--jobs", jobs.ToString(CultureInfo.InvariantCulture)]));
        }

        foreach (var (category, name, args) in scenarios)
        {
            var sw = Stopwatch.StartNew();
            var exit = await RunProcessAsync(weExecutable, args, projectRoot);
            sw.Stop();

            var result = new BenchmarkResult
            {
                Category = category,
                Scenario = name,
                ElapsedMs = sw.ElapsedMilliseconds,
                ExitCode = exit,
                Jobs = ParseJobsArg(args),
                Unity = args.Contains("--unity"),
                Config = "Development",
                Timestamp = DateTimeOffset.UtcNow
            };
            report.Results.Add(result);
            Log.Information("Benchmark {Category}/{Scenario}: {Ms}ms (exit {Exit})", category, name, sw.ElapsedMilliseconds, exit);
        }

        report.CompletedAt = DateTimeOffset.UtcNow;
        return report;
    }

    public static void WriteReports(BenchmarkReport report, string outputDirectory)
    {
        Directory.CreateDirectory(outputDirectory);
        var stamp = report.StartedAt.ToString("yyyyMMdd_HHmmss", CultureInfo.InvariantCulture);
        var baseName = $"benchmark_{stamp}";

        var jsonPath = Path.Combine(outputDirectory, $"{baseName}.json");
        File.WriteAllText(jsonPath, JsonSerializer.Serialize(report, new JsonSerializerOptions { WriteIndented = true }));

        var csvPath = Path.Combine(outputDirectory, $"{baseName}.csv");
        var csv = new StringBuilder();
        csv.AppendLine("timestamp,category,scenario,elapsed_ms,exit_code,jobs,unity,config");
        foreach (var r in report.Results)
        {
            csv.AppendLine(string.Join(',',
                r.Timestamp.ToString("O", CultureInfo.InvariantCulture),
                r.Category,
                r.Scenario,
                r.ElapsedMs.ToString(CultureInfo.InvariantCulture),
                r.ExitCode.ToString(CultureInfo.InvariantCulture),
                r.Jobs.ToString(CultureInfo.InvariantCulture),
                r.Unity ? "1" : "0",
                r.Config));
        }
        File.WriteAllText(csvPath, csv.ToString());

        var mdPath = Path.Combine(outputDirectory, $"{baseName}.md");
        File.WriteAllText(mdPath, BuildMarkdownReport(report));

        var htmlPath = Path.Combine(outputDirectory, $"{baseName}.html");
        File.WriteAllText(htmlPath, BuildHtmlReport(report));

        var latestMd = Path.Combine(outputDirectory, "latest.md");
        File.WriteAllText(latestMd, BuildMarkdownReport(report));

        Log.Information("Benchmark reports written to {Dir}", outputDirectory);
        Log.Information("  JSON: {Json}", jsonPath);
        Log.Information("  CSV:  {Csv}", csvPath);
        Log.Information("  MD:   {Md}", mdPath);
        Log.Information("  HTML: {Html}", htmlPath);
    }

    private static int ParseJobsArg(string[] args)
    {
        for (var i = 0; i < args.Length - 1; i++)
        {
            if (args[i] == "--jobs" && int.TryParse(args[i + 1], NumberStyles.Integer, CultureInfo.InvariantCulture, out var jobs))
                return jobs;
        }
        return Environment.ProcessorCount;
    }

    private static string BuildMarkdownReport(BenchmarkReport report)
    {
        var sb = new StringBuilder();
        sb.AppendLine("# IgniteBT Benchmark Report");
        sb.AppendLine();
        sb.AppendLine($"| Field | Value |");
        sb.AppendLine($"|-------|-------|");
        sb.AppendLine($"| Version | {report.IgniteBtVersion} |");
        sb.AppendLine($"| Started | {report.StartedAt:u} |");
        sb.AppendLine($"| Completed | {report.CompletedAt:u} |");
        sb.AppendLine($"| Duration | {(report.CompletedAt - report.StartedAt).TotalSeconds:F1}s |");
        sb.AppendLine($"| CPU cores | {report.ProcessorCount} |");
        sb.AppendLine($"| Project | `{report.ProjectRoot}` |");
        sb.AppendLine();
        sb.AppendLine("## Results");
        sb.AppendLine();
        sb.AppendLine("| Category | Scenario | Time (ms) | Exit | Jobs | Unity |");
        sb.AppendLine("|----------|----------|-----------|------|------|-------|");
        foreach (var r in report.Results)
            sb.AppendLine($"| {r.Category} | {r.Scenario} | {r.ElapsedMs} | {r.ExitCode} | {r.Jobs} | {(r.Unity ? "yes" : "no")} |");
        sb.AppendLine();

        var lifecycle = report.Results.Where(r => r.Category == "lifecycle" && r.ExitCode == 0).ToList();
        if (lifecycle.Count > 0)
        {
            sb.AppendLine("## Lifecycle Summary");
            sb.AppendLine();
            foreach (var r in lifecycle.OrderBy(r => r.ElapsedMs))
                sb.AppendLine($"- **{r.Scenario}**: {r.ElapsedMs} ms");
            sb.AppendLine();
        }

        var parallelism = report.Results.Where(r => r.Category == "parallelism" && r.ExitCode == 0).ToList();
        if (parallelism.Count > 1)
        {
            sb.AppendLine("## Parallelism");
            sb.AppendLine();
            sb.AppendLine("```");
            var maxMs = parallelism.Max(r => r.ElapsedMs);
            foreach (var r in parallelism.OrderBy(r => r.Jobs))
            {
                var barLen = maxMs > 0 ? (int)(r.ElapsedMs * 40 / maxMs) : 0;
                sb.AppendLine($"jobs={r.Jobs,-3} {new string('█', barLen)} {r.ElapsedMs}ms");
            }
            sb.AppendLine("```");
        }

        return sb.ToString();
    }

    private static string BuildHtmlReport(BenchmarkReport report)
    {
        var rows = string.Join('\n', report.Results.Select(r =>
            $"<tr><td>{r.Category}</td><td>{r.Scenario}</td><td>{r.ElapsedMs}</td><td>{r.ExitCode}</td><td>{r.Jobs}</td><td>{(r.Unity ? "yes" : "no")}</td></tr>"));

        var sb = new StringBuilder();
        sb.AppendLine("<!DOCTYPE html>");
        sb.AppendLine("<html lang=\"en\">");
        sb.AppendLine("<head>");
        sb.AppendLine("  <meta charset=\"utf-8\"/>");
        sb.AppendLine($"  <title>IgniteBT Benchmark — {report.StartedAt:yyyy-MM-dd HH:mm}</title>");
        sb.AppendLine("  <style>");
        sb.AppendLine("    body { font-family: system-ui, sans-serif; margin: 2rem; color: #1a1a1a; }");
        sb.AppendLine("    h1 { font-size: 1.5rem; }");
        sb.AppendLine("    table { border-collapse: collapse; width: 100%; max-width: 56rem; }");
        sb.AppendLine("    th, td { border: 1px solid #ccc; padding: 0.4rem 0.6rem; text-align: left; }");
        sb.AppendLine("    th { background: #f4f4f4; }");
        sb.AppendLine("    .meta { margin-bottom: 1.5rem; line-height: 1.6; }");
        sb.AppendLine("  </style>");
        sb.AppendLine("</head>");
        sb.AppendLine("<body>");
        sb.AppendLine("  <h1>IgniteBT Benchmark Report</h1>");
        sb.AppendLine("  <div class=\"meta\">");
        sb.AppendLine($"    <div>Version: <strong>{report.IgniteBtVersion}</strong></div>");
        sb.AppendLine($"    <div>Started: {report.StartedAt:u}</div>");
        sb.AppendLine($"    <div>CPU cores: {report.ProcessorCount}</div>");
        sb.AppendLine($"    <div>Project: <code>{EscapeHtml(report.ProjectRoot)}</code></div>");
        sb.AppendLine("  </div>");
        sb.AppendLine("  <table>");
        sb.AppendLine("    <thead><tr><th>Category</th><th>Scenario</th><th>Time (ms)</th><th>Exit</th><th>Jobs</th><th>Unity</th></tr></thead>");
        sb.AppendLine("    <tbody>");
        sb.AppendLine(rows);
        sb.AppendLine("    </tbody>");
        sb.AppendLine("  </table>");
        sb.AppendLine("</body>");
        sb.AppendLine("</html>");
        return sb.ToString();
    }

    private static async Task<int> RunProcessAsync(string exe, string[] args, string cwd)
    {
        var startInfo = new ProcessStartInfo
        {
            WorkingDirectory = cwd,
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };

        if (exe.Equals("dotnet", StringComparison.OrdinalIgnoreCase))
        {
            startInfo.FileName = "dotnet";
            startInfo.Arguments = "run --project Engine/Source/Programs/IgniteBT -- " + string.Join(' ', args);
        }
        else
        {
            startInfo.FileName = exe;
            startInfo.Arguments = string.Join(' ', args);
        }

        using var proc = new Process { StartInfo = startInfo };
        proc.Start();
        await proc.WaitForExitAsync();
        return proc.ExitCode;
    }

    private static string EscapeHtml(string value) =>
        value.Replace("&", "&amp;").Replace("<", "&lt;").Replace(">", "&gt;").Replace("\"", "&quot;");
}
