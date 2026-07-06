using IgniteBT.Build.Compiler;

namespace IgniteBT.CLI;

internal static class CommandLineHelpers
{
    public static bool TryReportErrors(ParsedCommand parsed)
    {
        if (parsed.IsValid)
        {
            return true;
        }

        foreach (var error in parsed.Errors)
        {
            Serilog.Log.Error("Invalid arguments: {Error}", error);
        }

        return false;
    }

    public static string NormalizePlatform(string? platform)
    {
        if (string.IsNullOrWhiteSpace(platform))
        {
            return GetCurrentPlatform();
        }

        return platform.Trim().ToLowerInvariant() switch
        {
            "win64" or "win32" or "windows" or "win" => "Windows",
            "linux" => "Linux",
            "mac" or "macos" or "osx" => "Mac",
            _ => platform
        };
    }

    public static BuildConfiguration ParseConfiguration(string config) =>
        config.ToLowerInvariant() switch
        {
            "debug" => BuildConfiguration.Debug,
            "development" or "dev" => BuildConfiguration.Development,
            "profile" => BuildConfiguration.Profile,
            "shipping" or "release" => BuildConfiguration.Shipping,
            _ => BuildConfiguration.Debug
        };

    public static int ParseJobs(string? jobsText)
    {
        if (string.IsNullOrWhiteSpace(jobsText))
        {
            return Environment.ProcessorCount;
        }

        if (!int.TryParse(jobsText, out var jobs) || jobs < 1)
        {
            throw new ArgumentException($"Invalid --jobs value '{jobsText}'. Expected a positive integer.");
        }

        return jobs;
    }

    public static string GetCurrentPlatform()
    {
        if (OperatingSystem.IsWindows())
        {
            return "Windows";
        }

        if (OperatingSystem.IsLinux())
        {
            return "Linux";
        }

        if (OperatingSystem.IsMacOS())
        {
            return "Mac";
        }

        return "Unknown";
    }
}
