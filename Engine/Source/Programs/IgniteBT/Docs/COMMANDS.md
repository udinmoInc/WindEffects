# 💻 IgniteBT Command Reference

> IgniteBT provides a comprehensive command-line interface accessed through the `we` launcher, offering a complete set of tools for building, managing, and diagnosing projects. This document describes all available commands, their parameters, and usage examples in detail. The command-line interface is designed to be intuitive for both new users and experienced developers, providing sensible defaults while allowing fine-grained control when needed. Commands are organized by functionality to make them easy to discover and understand, and each command includes comprehensive help documentation accessible through the `--help` flag.

---

## 📋 Command Overview

Commands are organized by functionality to provide a logical structure that reflects the different aspects of the build system:

| Category | Commands | Purpose |
|----------|----------|---------|
| 🔨 **Build Operations** | `build`, `clean`, `rebuild`, `package` | Create build artifacts and manage build process |
| ▶️ **Execution** | `run`, `daemon` | Execute programs and manage background services |
| 📁 **Project Management** | `project`, `plugin`, `modules` | Handle project-level operations |
| 🛠️ **SDK Management** | `sdk`, `setup` | SDK detection, validation, and environment setup |
| 🔍 **Diagnostics** | `doctor`, `version`, `graph`, `benchmark` | Diagnose issues and analyze performance |

---

## 🌐 Global Options

The following options apply to most commands and provide consistent behavior across the command-line interface:

| Option | Description | Default |
|--------|-------------|---------|
| `--config <CONFIGURATION>` | Build configuration (Debug, Development, Shipping) | Development |
| `--platform <PLATFORM>` | Target platform (Win64, Windows, Linux, Mac) | Win64 |
| `--target <TARGET>` | Specific build target or module to operate on | All targets |
| `--jobs <N>` | Number of parallel jobs to use | Auto-detected |
| `--verbose` | Enable detailed output | Disabled |
| `--help` | Display command-specific help information | - |

---

## 🔨 Build Commands

### build

The build command compiles the project or specified targets, transforming source code into executable binaries and libraries. This is the most commonly used command in IgniteBT and is the primary way to initiate builds. The build command analyzes the dependency graph to determine which files need to be compiled, invokes compilers with the appropriate flags, links the resulting object files, and produces the final build artifacts. The command supports incremental building, meaning that only files that have changed or are affected by changes are recompiled, significantly reducing build times for iterative development. The build process is highly parallelizable, with the system automatically determining which files can be compiled simultaneously and executing them in parallel across available CPU cores.

```powershell
we build [options]
```

#### Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--target <NAME>` | Build specific module or target | All targets |
| `--config <CONFIG>` | Build configuration | Development |
| `--platform <PLATFORM>` | Target platform | Win64 |
| `--jobs <N>` | Number of parallel jobs | Auto-detected |
| `--clean` | Clean artifacts before building | Disabled |
| `--verbose` | Enable detailed output | Disabled |
| `--report` | Generate build report with timing | Disabled |

#### Examples

```powershell
# Build entire project in Development configuration
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

The clean command removes build artifacts for a specified configuration or target, deleting the intermediate and output files produced by previous builds. This command is useful when you need to force a complete rebuild, typically when troubleshooting build issues or when major changes have been made that require starting from a clean state. The clean operation is selective based on the parameters provided, allowing you to clean specific configurations, targets, or platforms without affecting other build artifacts. The command removes files from the build directory but does not affect source code or configuration files, making it safe to run at any time.

```powershell
we clean [options]
```

#### Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--target <NAME>` | Clean specific module or target | All targets |
| `--config <CONFIG>` | Configuration to clean | All configurations |
| `--platform <PLATFORM>` | Platform to clean | All platforms |
| `--verbose` | Show files being deleted | Disabled |

#### Examples

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

The rebuild command performs a clean and build operation in a single step, combining the functionality of the clean and build commands. This is a convenience command that is commonly used when you need to force a complete rebuild of the project or a specific target. The rebuild command first removes the build artifacts for the specified target and then initiates a fresh build, ensuring that all files are recompiled from scratch. This is particularly useful when troubleshooting build issues that may be caused by stale or corrupted build artifacts, or when configuration changes require a complete rebuild to take effect.

```powershell
we rebuild [options]
```

#### Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--target <NAME>` | Rebuild specific module or target | All targets |
| `--config <CONFIG>` | Build configuration | Development |
| `--platform <PLATFORM>` | Target platform | Win64 |
| `--jobs <N>` | Number of parallel jobs | Auto-detected |
| `--verbose` | Enable detailed output | Disabled |

#### Examples

```powershell
# Rebuild entire project
we rebuild --config Development

# Rebuild specific module
we rebuild --target WECore --config Debug
```

---

### package

The package command packages build artifacts for distribution, creating distributable packages containing the built binaries, libraries, and other required files. This command is typically used when preparing releases or when you need to distribute build artifacts to other machines or users. The packaging process collects the relevant build outputs, organizes them according to the target platform and configuration, and creates a package in a format suitable for distribution. The command can package entire projects or specific targets, allowing flexibility in what gets included in the distribution.

```powershell
we package [options]
```

#### Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--target <NAME>` | Package specific target | All targets |
| `--config <CONFIG>` | Build configuration to package | Shipping |
| `--platform <PLATFORM>` | Target platform | All platforms |

#### Examples

```powershell
# Package shipping build
we package --config Shipping
```

---

## ▶️ Execution Commands

### run

The run command executes a built target, typically the editor or game executable, launching it with the appropriate configuration and environment. This command is used to run the programs that have been built by the build system, providing a convenient way to launch the editor, game, or other executables without needing to manually locate and execute them. The run command automatically locates the built executable based on the specified target and configuration, sets up the required environment, and launches the program. This ensures that the executable runs with the correct working directory, environment variables, and other settings required for proper operation.

```powershell
we run [options]
```

#### Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--target <NAME>` | Target to run | Editor |
| `--config <CONFIG>` | Build configuration to run | Development |
| `--args <ARGUMENTS>` | Command-line arguments for target | None |

#### Examples

```powershell
# Run editor in Development configuration
we run --target Editor --config Development

# Run with custom arguments
we run --target Editor --config Development --args "-project MyProject"
```

---

### daemon

The daemon command starts the IgniteBT daemon for background build operations and distributed compilation coordination. The daemon is a background process that provides build services without requiring manual initiation of each build operation. This is particularly useful for distributed builds, where the daemon coordinates work across multiple machines, and for continuous integration scenarios where builds need to run automatically. The daemon listens for build requests, manages worker processes, and handles the complexity of distributed build coordination. Running the daemon can improve build efficiency by keeping build processes warm and ready to execute builds quickly.

```powershell
we daemon [options]
```

#### Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--port <PORT>` | Port number for daemon to listen on | Default port |
| `--workers <N>` | Number of worker processes to spawn | Auto-detected |

#### Examples

```powershell
# Start daemon with default settings
we daemon

# Start daemon with custom port
we daemon --port 8080
```

---

## 📁 Project Management Commands

### project

The project command manages project files and project-level operations, providing tools for creating, listing, and opening projects within the workspace. This command is essential for working with multiple projects or for managing the project structure within a larger codebase. The project command handles the complexity of project file management, ensuring that projects are properly configured and that the build system can locate and work with them correctly. Projects in IgniteBT represent logical groupings of work, such as different games, applications, or components that share a common codebase but have distinct build requirements.

```powershell
we project <action> [options]
```

#### Actions

| Action | Description |
|--------|-------------|
| `list` | List all projects in the workspace |
| `open <NAME>` | Open specified project as active |
| `create <NAME>` | Create new project with configuration |

#### Examples

```powershell
# List all projects
we project list

# Open specific project
we project open MyProject

# Create new project
we project create NewProject
```

---

### plugin

The plugin command manages build plugins and extensions, providing tools for listing, building, enabling, and disabling plugins that extend the functionality of the build system. Plugins are modular extensions that can add new commands, modify build behavior, or integrate with external tools and services. The plugin system allows the build system to be customized and extended without modifying the core codebase, making it possible to adapt the build system to specific project requirements or to integrate with organization-specific tools and workflows. Plugins can be developed by the project team or obtained from third-party sources, providing a flexible extension mechanism.

```powershell
we plugin <action> [options]
```

#### Actions

| Action | Description |
|--------|-------------|
| `list` | List all available plugins and their status |
| `build <NAME>` | Build specified plugin |
| `enable <NAME>` | Enable specified plugin |
| `disable <NAME>` | Disable specified plugin |

#### Examples

```powershell
# List available plugins
we plugin list

# Build specific plugin
we plugin build MyPlugin

# Enable plugin
we plugin enable MyPlugin
```

---

### modules

The modules command lists and manages build modules, providing visibility into the module structure of the project and the relationships between modules. Modules are the fundamental building blocks of projects in IgniteBT, representing logical groupings of source code that are built together. Understanding the module structure is important for navigating large codebases and for understanding the impact of changes. The modules command can display simple lists of modules, dependency graphs showing how modules depend on each other, or hierarchical trees showing the organization of modules within the project.

```powershell
we modules [options]
```

#### Parameters

| Parameter | Description |
|-----------|-------------|
| `--graph` | Display module dependency graph |
| `--tree` | Display module hierarchy |

#### Examples

```powershell
# List all modules
we modules

# Display dependency graph
we modules --graph

# Display module hierarchy
we modules --tree
```

---

## 🛠️ SDK Management Commands

### sdk

The sdk command manages and validates development SDKs, providing tools for detecting, listing, and validating the software development kits required for building the project. SDKs are external libraries and tools that the project depends on, such as graphics APIs, windowing systems, and other third-party libraries. Proper SDK management is critical for ensuring that builds are reproducible and that all developers have the required dependencies installed. The sdk command automates the process of detecting installed SDKs, validating their versions against project requirements, and identifying any missing or misconfigured SDKs.

```powershell
we sdk <action> [options]
```

#### Actions

| Action | Description |
|--------|-------------|
| `list` | List all detected SDKs with versions and paths |
| `detect` | Perform fresh detection of installed SDKs |
| `validate` | Validate SDK configuration against requirements |

#### Examples

```powershell
# List all detected SDKs
we sdk list

# Detect installed SDKs
we sdk detect

# Validate SDK configuration
we sdk validate
```

---

### setup

The setup command configures the development environment and installs the `we` command globally, providing a streamlined way to get started with IgniteBT. This command handles the initial configuration required to use the build system, including setting up the command-line launcher, configuring environment variables, and validating that all prerequisites are met. The setup command is typically run once when first setting up a development environment, but it can also be run to reconfigure or repair the setup if needed. The command provides clear feedback about what it is doing and reports any issues that need to be addressed.

```powershell
we setup [options]
```

#### Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--global` | Install `we` command globally to PATH | Disabled |
| `--path <PATH>` | Custom installation path | Default location |

#### Examples

```powershell
# Configure development environment
we setup

# Install we command globally
we setup --global
```

---

## 🔍 Diagnostic Commands

### doctor

The doctor command performs comprehensive environment and dependency diagnostics, providing a thorough health check of the build environment. This command is invaluable for troubleshooting build issues, validating environment setup, and ensuring that all prerequisites are properly configured. The doctor command checks a wide range of factors including SDK availability and version compatibility, tool installation and configuration, file system permissions, environment variable settings, and dependency integrity. The command provides clear, actionable feedback about any issues it detects, making it easier to identify and resolve problems before they cause build failures.

```powershell
we doctor [options]
```

#### Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--verbose` | Enable detailed diagnostic output | Disabled |
| `--fix` | Attempt to automatically fix detected issues | Disabled |

#### Examples

```powershell
# Run diagnostics
we doctor

# Run with detailed output
we doctor --verbose

# Attempt to fix issues
we doctor --fix
```

---

### version

The version command displays IgniteBT version information, showing the current version of the build system that is installed. This command is useful for verifying which version of the build system is being used, which is important for ensuring compatibility and for troubleshooting issues that may be version-specific. The version information includes both the version number and the product name, providing clear identification of the build system.

```powershell
we version
```

#### Output

```
IgniteBT v1.0.0
WindEffects Build Tool
```

---

### graph

The graph command generates and displays the build dependency graph, providing a visual representation of how different modules and targets depend on each other. The dependency graph is a fundamental data structure in the build system, and visualizing it can be invaluable for understanding the project structure, identifying potential circular dependencies, and understanding the impact of changes. The graph can be displayed in various formats and can be scoped to specific targets or modules, allowing focused analysis of particular parts of the dependency graph.

```powershell
we graph [options]
```

#### Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--output <FILE>` | Output file path for graph | Console output |
| `--format <FORMAT>` | Output format (dot, svg, png) | Text |
| `--target <NAME>` | Graph specific target only | Entire project |

#### Examples

```powershell
# Display dependency graph
we graph

# Export to DOT format
we graph --output dependencies.dot --format dot

# Graph specific target
we graph --target WECore
```

---

### benchmark

The benchmark command runs build performance benchmarks, measuring and analyzing the performance of the build system under various conditions. This command is useful for identifying performance bottlenecks, optimizing build configuration, and understanding how different settings affect build times. The benchmark command can test different levels of parallelism, measure the impact of caching, and compare performance across different configurations. The results provide actionable insights into how to optimize the build process for maximum efficiency.

```powershell
we benchmark [options]
```

#### Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--jobs <N>` | Test with specific job count(s) | Default |
| `--iterations <N>` | Number of benchmark iterations | 5 |
| `--target <NAME>` | Benchmark specific target | Entire project |

#### Examples

```powershell
# Run default benchmarks
we benchmark

# Test with different job counts
we benchmark --jobs 8,16,32

# Benchmark specific target
we benchmark --target WECore
```

---

## 🔄 Common Workflows

### 🚀 Initial Project Setup

The initial project setup workflow prepares a new development environment for building with IgniteBT. This workflow should be followed when setting up a new machine or when starting work on a new project.

```powershell
# Configure environment
we setup

# Validate SDKs
we sdk validate

# Initial build
we build --config Development
```

> **💡 Tip:** Run this workflow once when setting up a new development environment to ensure everything is correctly configured.

---

### 🔄 Development Cycle

The development cycle workflow represents the typical iterative process used during active development. This cycle balances the need for quick iteration with the need to ensure that changes are correctly integrated.

```powershell
# Incremental build
we build --config Development

# Run editor
we run --target Editor --config Development

# Clean and rebuild if needed
we rebuild --config Debug
```

> **⚡ Note:** Incremental builds are the most common operation during development and provide fast feedback.

---

### 📦 Release Preparation

The release preparation workflow is used when preparing a build for distribution or deployment. This workflow ensures that the build is optimized, clean, and properly packaged for distribution.

```powershell
# Clean all artifacts
we clean

# Build shipping configuration
we build --config Shipping --jobs 16

# Package for distribution
we package --config Shipping
```

> **🎯 Best Practice:** Always clean before release builds to ensure no stale artifacts are included.

---

### 🔧 Troubleshooting

The troubleshooting workflow is used when encountering build issues or unexpected behavior. This workflow systematically diagnoses and resolves problems by checking the environment, validating configuration, and performing clean rebuilds.

```powershell
# Run diagnostics
we doctor --verbose

# Validate SDK configuration
we sdk validate

# Clean and rebuild with verbose output
we rebuild --config Debug --verbose
```

> **🔍 Diagnostic Tip:** Use `--verbose` flag to get detailed information about what's being checked and why it might be failing.

---

## 📊 Exit Codes

IgniteBT uses standardized exit codes to indicate the result of command execution. These exit codes can be used in scripts and automation to determine whether a command succeeded or failed and to take appropriate action based on the type of failure.

| Exit Code | Meaning | Description |
|-----------|---------|-------------|
| `0` | ✅ Success | Command completed without errors |
| `1` | ❌ General Error | Error that doesn't fit other categories |
| `2` | 🔨 Build Failure | Compilation or linking process failed |
| `3` | ⚙️ Configuration Error | Problem with build system configuration |
| `4` | 🔗 Dependency Error | Required dependencies could not be resolved |
| `5` | 🛠️ SDK Not Found | Required SDK is missing or cannot be located |

---

## 🌍 Environment Variables

IgniteBT respects several environment variables that allow customization of default behavior without needing to specify options on every command.

| Variable | Description | Example |
|----------|-------------|---------|
| `IGNITEBT_CONFIG` | Default build configuration | `Development` |
| `IGNITEBT_PLATFORM` | Default target platform | `Win64` |
| `IGNITEBT_JOBS` | Default parallel job count | `8` |
| `IGNITEBT_VERBOSE` | Enable verbose output globally | `1` |
| `IGNITEBT_BUILD_ROOT` | Custom build directory path | `C:\Build` |

> **💡 Usage:** Set these variables in your shell profile or CI/CD configuration to establish project-wide defaults.

---

## 📄 Configuration Files

IgniteBT uses several configuration files to manage different aspects of the build system.

| File | Purpose | Location |
|------|---------|----------|
| `.engine` | Project descriptor defining structure and configuration | Project root |
| `.SDKs.json` | SDK path configuration and overrides | Project root |
| `bootstrap.manifest` | Build system bootstrap configuration | Auto-generated |

---

## ❓ Additional Help

For command-specific help, use the help flag with any command to display detailed usage information.

```powershell
we <command> --help
```

This displays detailed usage information for the specified command, including all available parameters, their descriptions, and examples of common usage patterns. The help information is automatically generated from the command metadata, ensuring that it is always consistent with the actual command implementation.

---

## 🔗 Related Documentation

- 📖 [Getting Started Guide](./GETTING-STARTED.md)
- 🏗️ [Architecture Documentation](./ARCHITECTURE.md)
- ⚙️ [Configuration Guide](./CONFIGURATION.md)
- 📋 [Changelog](../../../../../CHANGELOG.md)
