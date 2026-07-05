namespace IgniteBT.CLI;

/// <summary>
/// Describes a command-line option or flag.
/// </summary>
public sealed class OptionSpec
{
    public required string LongName { get; init; }
    public char? ShortName { get; init; }
    public bool RequiresValue { get; init; }
    public string? DefaultValue { get; init; }
    public string Description { get; init; } = string.Empty;
}

/// <summary>
/// Schema for a subcommand's options and positionals.
/// </summary>
public sealed class CommandSchema
{
    private readonly Dictionary<string, OptionSpec> _optionsByLong = new(StringComparer.OrdinalIgnoreCase);
    private readonly Dictionary<char, string> _optionsByShort = new();
    private readonly List<string> _positionalNames = new();

    public string CommandName { get; }

    public CommandSchema(string commandName)
    {
        CommandName = commandName;
    }

    public CommandSchema WithOption(string longName, char? shortName = null, string? defaultValue = null, string description = "")
    {
        var spec = new OptionSpec
        {
            LongName = longName,
            ShortName = shortName,
            RequiresValue = true,
            DefaultValue = defaultValue,
            Description = description
        };

        _optionsByLong[longName] = spec;
        if (shortName.HasValue)
        {
            _optionsByShort[shortName.Value] = longName;
        }

        return this;
    }

    public CommandSchema WithFlag(string longName, char? shortName = null, string description = "")
    {
        var spec = new OptionSpec
        {
            LongName = longName,
            ShortName = shortName,
            RequiresValue = false,
            DefaultValue = null,
            Description = description
        };

        _optionsByLong[longName] = spec;
        if (shortName.HasValue)
        {
            _optionsByShort[shortName.Value] = longName;
        }

        return this;
    }

    public CommandSchema WithPositional(string name)
    {
        _positionalNames.Add(name);
        return this;
    }

    public ParsedCommand Parse(string[] args)
    {
        var errors = new List<string>();
        var values = new Dictionary<string, string?>(StringComparer.OrdinalIgnoreCase);
        var flags = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var positionals = new List<string>();

        foreach (var spec in _optionsByLong.Values)
        {
            if (!spec.RequiresValue && spec.DefaultValue is null)
            {
                continue;
            }

            if (spec.DefaultValue is not null)
            {
                values[spec.LongName] = spec.DefaultValue;
            }
        }

        for (var i = 0; i < args.Length; i++)
        {
            var token = args[i];

            if (token == "--")
            {
                positionals.AddRange(args[(i + 1)..]);
                break;
            }

            if (token.StartsWith("--", StringComparison.Ordinal))
            {
                var body = token[2..];
                if (body.Length == 0)
                {
                    errors.Add("empty option '--' is not supported");
                    continue;
                }

                string optionName;
                string? inlineValue = null;

                var equalsIndex = body.IndexOf('=');
                if (equalsIndex >= 0)
                {
                    optionName = body[..equalsIndex];
                    inlineValue = body[(equalsIndex + 1)..];
                }
                else
                {
                    optionName = body;
                }

                if (!_optionsByLong.TryGetValue(optionName, out var spec))
                {
                    errors.Add($"unknown option '--{optionName}'");
                    continue;
                }

                if (!spec.RequiresValue)
                {
                    flags.Add(spec.LongName);
                    values[spec.LongName] = "true";
                    continue;
                }

                string? value = inlineValue;
                if (value is null)
                {
                    if (i + 1 >= args.Length)
                    {
                        errors.Add($"option '--{optionName}' requires a value");
                        continue;
                    }

                    var next = args[i + 1];
                    if (IsOptionToken(next))
                    {
                        errors.Add($"option '--{optionName}' requires a value");
                        continue;
                    }

                    value = next;
                    i++;
                }

                values[spec.LongName] = value;
                continue;
            }

            if (token.StartsWith("-", StringComparison.Ordinal) && token.Length > 1)
            {
                if (token.Length == 2)
                {
                    var shortName = token[1];
                    if (!_optionsByShort.TryGetValue(shortName, out var longName) ||
                        !_optionsByLong.TryGetValue(longName, out var spec))
                    {
                        errors.Add($"unknown option '-{shortName}'");
                        continue;
                    }

                    if (!spec.RequiresValue)
                    {
                        flags.Add(spec.LongName);
                        values[spec.LongName] = "true";
                        continue;
                    }

                    if (i + 1 >= args.Length)
                    {
                        errors.Add($"option '-{shortName}' requires a value");
                        continue;
                    }

                    var next = args[i + 1];
                    if (IsOptionToken(next))
                    {
                        errors.Add($"option '-{shortName}' requires a value");
                        continue;
                    }

                    values[spec.LongName] = next;
                    i++;
                    continue;
                }

                // Clustered short flags, e.g. -Cj (only flags, no values in cluster)
                for (var j = 1; j < token.Length; j++)
                {
                    var shortName = token[j];
                    if (!_optionsByShort.TryGetValue(shortName, out var longName) ||
                        !_optionsByLong.TryGetValue(longName, out var spec))
                    {
                        errors.Add($"unknown option '-{shortName}' in '{token}'");
                        continue;
                    }

                    if (spec.RequiresValue)
                    {
                        errors.Add($"option '-{shortName}' requires a value and cannot be clustered in '{token}'");
                        continue;
                    }

                    flags.Add(spec.LongName);
                    values[spec.LongName] = "true";
                }

                continue;
            }

            positionals.Add(token);
        }

        return new ParsedCommand
        {
            Command = CommandName,
            Options = values,
            Flags = flags,
            Positionals = positionals,
            PositionalNames = _positionalNames.ToArray(),
            Errors = errors
        };
    }

    private static bool IsOptionToken(string token) =>
        token.StartsWith("-", StringComparison.Ordinal);
}

/// <summary>
/// Parsed command-line arguments.
/// </summary>
public sealed class ParsedCommand
{
    public required string Command { get; init; }
    public required IReadOnlyDictionary<string, string?> Options { get; init; }
    public required IReadOnlySet<string> Flags { get; init; }
    public required IReadOnlyList<string> Positionals { get; init; }
    public required IReadOnlyList<string> PositionalNames { get; init; }
    public required IReadOnlyList<string> Errors { get; init; }

    public bool IsValid => Errors.Count == 0;

    public string GetOption(string name, string defaultValue = "")
    {
        if (Options.TryGetValue(name, out var value) && !string.IsNullOrWhiteSpace(value))
        {
            return value;
        }

        return defaultValue;
    }

    public bool HasFlag(string name) => Flags.Contains(name);

    public string ResolveTarget(string defaultTarget = "Default")
    {
        var explicitTarget = GetOption("target", string.Empty);
        if (!string.IsNullOrWhiteSpace(explicitTarget))
        {
            return explicitTarget;
        }

        if (Positionals.Count > 0)
        {
            return Positionals[0];
        }

        return defaultTarget;
    }
}

/// <summary>
/// Shared schemas for IgniteBT commands.
/// </summary>
public static class CommandSchemas
{
    public static readonly CommandSchema Build = new CommandSchema("build")
        .WithPositional("target")
        .WithOption("config", 'c', defaultValue: "Release", description: "Build configuration")
        .WithOption("platform", 'p', description: "Target platform")
        .WithOption("target", 't', description: "Module or target to build (Editor, Engine, Renderer)")
        .WithOption("jobs", 'j', description: "Parallel compile jobs")
        .WithOption("unity-size", description: "Max files per unity blob")
        .WithOption("unity-disable", description: "Comma-separated modules to exclude from unity builds")
        .WithFlag("clean", 'C', description: "Clean before building")
        .WithFlag("unity", description: "Enable unity builds");

    public static readonly CommandSchema Clean = new CommandSchema("clean")
        .WithPositional("target")
        .WithOption("config", 'c', defaultValue: "Debug", description: "Build configuration")
        .WithOption("platform", 'p', description: "Target platform")
        .WithOption("target", 't', description: "Module or target to clean");

    public static readonly CommandSchema Rebuild = new CommandSchema("rebuild")
        .WithPositional("target")
        .WithOption("config", 'c', defaultValue: "Debug", description: "Build configuration")
        .WithOption("platform", 'p', description: "Target platform")
        .WithOption("target", 't', description: "Module or target to rebuild")
        .WithOption("jobs", 'j', description: "Parallel compile jobs")
        .WithFlag("clean", 'C', description: "Ignored for rebuild; clean always runs first")
        .WithOption("unity-size", description: "Max files per unity blob")
        .WithFlag("unity", description: "Enable unity builds");

    public static readonly CommandSchema Run = new CommandSchema("run")
        .WithPositional("target")
        .WithOption("config", 'c', defaultValue: "Debug", description: "Build configuration")
        .WithOption("target", 't', defaultValue: "Editor", description: "Executable target to run");
}
