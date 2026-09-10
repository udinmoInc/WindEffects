# IgniteBT Command Reference

IgniteBT provides a command-line interface through the `we` launcher for building, managing, and diagnosing projects.

---

## Command Overview

| Category | Commands | Purpose |
|----------|----------|---------|
| Build Operations | `build`, `clean`, `rebuild`, `package` | Create and manage build artifacts |
| Execution | `run`, `daemon` | Execute programs and background services |
| Project Management | `project`, `plugin`, `modules` | Project-level operations |
| SDK Management | `sdk`, `setup` | SDK detection, validation, environment setup |
| Diagnostics | `doctor`, `version`, `graph`, `benchmark` | Diagnose issues and analyze performance |

---

## Global Options

| Option | Description | Default |
|--------|-------------|---------|
| `--config <CONFIGURATION>` | Build configuration (Debug, Development, Shipping) | Development |
| `--platform <PLATFORM>` | Target platform (Win64, Windows, Linux, Mac) | Win64 |
| `--target <TARGET>` | Specific build target or module | All targets |
| `--jobs <N>` | Number of parallel jobs | Auto-detected |
| `--verbose` | Enable detailed output | Disabled |
| `--help` | Display command-specific help | - |

---

## Build Commands

### build

Compiles the project or specified targets. Analyzes the dependency graph, invokes compilers, links object files, and produces build artifacts. Supports incremental building.

```powershell
we build [options]
```

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--target <NAME>` | Build specific module or target | All targets |
| `--config <CONFIG>` | Build configuration | Development |
| `--platform <PLATFORM>` | Target platform | Win64 |
| `--jobs <N>` | Number of parallel jobs | Auto-detected |
| `--clean` | Clean artifacts before building | Disabled |
| `--verbose` | Enable detailed output | Disabled |
| `--report` | Generate build report with timing | Disabled |

**Examples:**
```powershell
# Build entire project in Development
we build --config Development

# Build specific module
we build --target WECore --config Debug

# Build with increased parallelism
we build --config Shipping --jobs 16

# Clean build with verbose output
we build --config Debug --clean --verbose
```

---

### clean

Removes build artifacts for specified configuration or target. Selective based on parameters.

```powershell
we clean [options]
```

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--target <NAME>` | Clean specific module or target | All targets |
| `--config <CONFIG>` | Configuration to clean | All configurations |
| `--platform <PLATFORM>` | Platform to clean | All platforms |
| `--verbose` | Show files being deleted | Disabled |

**Examples:**
```powershell
# Clean all Debug artifacts
we clean --config Debug

# Clean specific module
we clean --target WERenderer --config Development

# Clean all configurations
we clean
```

---

### rebuild

Performs clean and build in a single step.

```powershell
we rebuild [options]
```

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--target <NAME>` | Rebuild specific module or target | All targets |
| `--config <CONFIG>` | Build configuration | Development |
| `--platform <PLATFORM>` | Target platform | Win64 |
| `--jobs <N>` | Number of parallel jobs | Auto-detected |
| `--verbose` | Enable detailed output | Disabled |

**Examples:**
```powershell
# Rebuild entire project
we rebuild --config Development

# Rebuild specific module
we rebuild --target WECore --config Debug
```

---

### package

Packages build artifacts for distribution.

```powershell
we package [options]
```

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--target <NAME>` | Package specific target | All targets |
| `--config <CONFIG>` | Build configuration to package | Shipping |
| `--platform <PLATFORM>` | Target platform | All platforms |

**Examples:**
```powershell
# Package shipping build
we package --config Shipping
```

---

## Execution Commands

### run

Executes a built target (editor, game, etc.).

```powershell
we run [options]
```

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--target <NAME>` | Target to run | Editor |
| `--config <CONFIG>` | Build configuration to run | Development |
| `--args <ARGUMENTS>` | Command-line arguments for target | None |

**Examples:**
```powershell
# Run editor in Development
we run --target Editor --config Development

# Run with custom arguments
we run --target Editor --config Development --args "-project MyProject"
```

---

### daemon

Starts the IgniteBT daemon for background build operations and distributed compilation.

```powershell
we daemon [options]
```

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--port <PORT>` | Port number for daemon | Default port |
| `--workers <N>` | Number of worker processes | Auto-detected |

**Examples:**
```powershell
# Start daemon with defaults
we daemon

# Start with custom port
we daemon --port 8080
```

---

## Project Management Commands

### project

Manages project files and project-level operations.

```powershell
we project <action> [options]
```

| Action | Description |
|--------|-------------|
| `list` | List all projects in workspace |
| `open <NAME>` | Open specified project as active |
| `create <NAME>` | Create new project with configuration |

**Examples:**
```powershell
we project list
we project open MyProject
we project create NewProject
```

---

### plugin

Manages build plugins and extensions.

```powershell
we plugin <action> [options]
```

| Action | Description |
|--------|-------------|
| `list` | List all available plugins and status |
| `build <NAME>` | Build specified plugin |
| `enable <NAME>` | Enable specified plugin |
| `disable <NAME>` | Disable specified plugin |

**Examples:**
```powershell
we plugin list
we plugin build MyPlugin
we plugin enable MyPlugin
```

---

### modules

Lists and manages build modules.

```powershell
we modules [options]
```

| Parameter | Description |
|-----------|-------------|
| `--graph` | Display module dependency graph |
| `--tree` | Display module hierarchy |

**Examples:**
```powershell
we modules
we modules --graph
we modules --tree
```

---

## SDK Management Commands

### sdk

Manages and validates development SDKs.

```powershell
we sdk <action> [options]
```

| Action | Description |
|--------|-------------|
| `list` | List all detected SDKs with versions and paths |
| `detect` | Perform fresh detection of installed SDKs |
| `validate` | Validate SDK configuration against requirements |

**Examples:**
```powershell
we sdk list
we sdk detect
we sdk validate
```

---

### setup

Configures the development environment and installs `we` globally.

```powershell
we setup [options]
```

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--global` | Install `we` command globally to PATH | Disabled |
| `--path <PATH>` | Custom installation path | Default location |

**Examples:**
```powershell
we setup
we setup --global
```

---

## Diagnostic Commands

### doctor

Performs comprehensive environment and dependency diagnostics.

```powershell
we doctor [options]
```

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--verbose` | Enable detailed diagnostic output | Disabled |
| `--fix` | Attempt to automatically fix issues | Disabled |

**Examples:**
```powershell
we doctor
we doctor --verbose
we doctor --fix
```

---

### version

Displays IgniteBT version information.

```powershell
we version
```

**Output:**
```
IgniteBT v1.0.0
WindEffects Build Tool
```

---

### graph

Generates and displays the build dependency graph.

```powershell
we graph [options]
```

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--output <FILE>` | Output file path | Console |
| `--format <FORMAT>` | Output format (dot, svg, png) | Text |
| `--target <NAME>` | Graph specific target only | Entire project |

**Examples:**
```powershell
we graph
we graph --output dependencies.dot --format dot
we graph --target WECore
```

---

### benchmark

Runs build performance benchmarks.

```powershell
we benchmark [options]
```

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--jobs <N>` | Test with specific job count(s) | Default |
| `--iterations <N>` | Number of benchmark iterations | 5 |
| `--target <NAME>` | Benchmark specific target | Entire project |

**Examples:**
```powershell
we benchmark
we benchmark --jobs 8,16,32
we benchmark --target WECore
```

---

## Common Workflows

### Initial Project Setup

```powershell
we setup
we sdk validate
we build --config Development
```

Run once when setting up a new development environment.

---

### Development Cycle

```powershell
we build --config Development
we run --target Editor --config Development
we rebuild --config Debug  # if needed
```

Incremental builds provide fast feedback.

---

### Release Preparation

```powershell
we clean
we build --config Shipping --jobs 16
we package --config Shipping
```

Always clean before release builds to ensure no stale artifacts.

---

### Troubleshooting

```powershell
we doctor --verbose
we sdk validate
we rebuild --config Debug --verbose
```

Use `--verbose` for detailed diagnostic information.

---

## Exit Codes

| Exit Code | Meaning | Description |
|-----------|---------|-------------|
| `0` | Success | Command completed without errors |
| `1` | General Error | Error not fitting other categories |
| `2` | Build Failure | Compilation or linking failed |
| `3` | Configuration Error | Build system configuration problem |
| `4` | Dependency Error | Required dependencies unresolved |
| `5` | SDK Not Found | Required SDK missing or not located |

---

## Environment Variables

| Variable | Description | Example |
|----------|-------------|---------|
| `IGNITEBT_CONFIG` | Default build configuration | `Development` |
| `IGNITEBT_PLATFORM` | Default target platform | `Win64` |
| `IGNITEBT_JOBS` | Default parallel job count | `8` |
| `IGNITEBT_VERBOSE` | Enable verbose output globally | `1` |
| `IGNITEBT_BUILD_ROOT` | Custom build directory path | `C:\Build` |

Set in shell profile or CI/CD configuration for project-wide defaults.

---

## Configuration Files

| File | Purpose | Location |
|------|---------|----------|
| `.engine` | Project descriptor | Project root |
| `IgniteBT.SDKs.json` | SDK path configuration | Project root |
| `bootstrap.manifest` | Bootstrap configuration | Auto-generated |

---

## Additional Help

For command-specific help:

```powershell
we <command> --help
```

Displays detailed usage, parameters, and examples for the specified command.

---

## Related Documentation

- [Getting Started Guide](./GETTING-STARTED.md)
- [Architecture Documentation](./ARCHITECTURE.md)
- [Configuration Guide](./CONFIGURATION.md)
- [Changelog](../../../../../CHANGELOG.md)