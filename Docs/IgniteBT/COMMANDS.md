# IgniteBT Command Reference

IgniteBT provides a comprehensive command-line interface accessed through the `we` launcher, offering a complete set of tools for building, managing, and diagnosing projects. This document describes all available commands, their parameters, and usage examples in detail. The command-line interface is designed to be intuitive for both new users and experienced developers, providing sensible defaults while allowing fine-grained control when needed. Commands are organized by functionality to make them easy to discover and understand, and each command includes comprehensive help documentation accessible through the `--help` flag.

## Command Overview

Commands are organized by functionality to provide a logical structure that reflects the different aspects of the build system. Build Operations include `build`, `clean`, `rebuild`, and `package`, which are the core commands for creating build artifacts and managing the build process. Execution commands include `run` and `daemon`, which are used to execute built programs and manage background build services. Project Management commands include `project`, `plugin`, and `modules`, which handle project-level operations such as project creation, plugin management, and module inspection. SDK Management commands include `sdk` and `setup`, which handle SDK detection, validation, and environment setup. Diagnostic commands include `doctor`, `version`, `graph`, and `benchmark`, which provide tools for diagnosing issues, displaying version information, visualizing dependency graphs, and measuring build performance.

## Global Options

The following options apply to most commands and provide consistent behavior across the command-line interface. The `--config <CONFIGURATION>` option specifies the build configuration to use, with valid values being Debug, Development, or Shipping. This option determines the optimization level, debug information inclusion, and other build characteristics. The `--platform <PLATFORM>` option specifies the target platform, with valid values including Win64, Windows, Linux, and Mac. This option controls which platform-specific code and libraries are included in the build. The `--target <TARGET>` option specifies a particular build target or module to operate on, allowing commands to be scoped to specific parts of the project rather than operating on the entire project. The `--jobs <N>` option controls the number of parallel jobs to use, allowing users to tune the level of parallelism based on their system capabilities and build requirements. The `--verbose` option enables detailed output, providing additional information about command execution that can be useful for troubleshooting and understanding what the command is doing. The `--help` option displays command-specific help information, including usage syntax, parameter descriptions, and examples.

## Build Commands

### build

The build command compiles the project or specified targets, transforming source code into executable binaries and libraries. This is the most commonly used command in IgniteBT and is the primary way to initiate builds. The build command analyzes the dependency graph to determine which files need to be compiled, invokescompilers with the appropriate flags, links the resulting object files, and produces the final build artifacts. The command supports incremental building, meaning that only files that have changed or are affected by changes are recompiled, significantly reducing build times for iterative development. The build process is highly parallelizable, with the system automatically determining which files can be compiled simultaneously and executing them in parallel across available CPU cores.

```powershell
we build [options]
```

The build command accepts several parameters to control its behavior. The `--target <NAME>` parameter allows building a specific module or target rather than the entire project, which is useful for focusing on a particular component during development. The `--config <CONFIG>` parameter specifies the build configuration, with Development being the default. Configuration determines optimization level, debug information inclusion, and other build characteristics. The `--platform <PLATFORM>` parameter specifies the target platform, with Win64 being the default. This controls which platform-specific code and libraries are included. The `--jobs <N>` parameter controls the number of parallel jobs to use, allowing tuning of parallelism based on system capabilities. The `--clean` flag causes the command to clean build artifacts before building, forcing a complete rebuild. The `--verbose` flag enables detailed output showing the exact commands being executed and their results. The `--report` flag generates a build report with timing information and other statistics.

**Examples:**

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

### clean

The clean command removes build artifacts for a specified configuration or target, deleting the intermediate and output files produced by previous builds. This command is useful when you need to force a complete rebuild, typically when troubleshooting build issues or when major changes have been made that require starting from a clean state. The clean operation is selective based on the parameters provided, allowing you to clean specific configurations, targets, or platforms without affecting other build artifacts. The command removes files from the build directory but does not affect source code or configuration files, making it safe to run at any time.

```powershell
we clean [options]
```

The clean command accepts parameters to control what gets cleaned. The `--target <NAME>` parameter allows cleaning a specific module or target, removing only the artifacts associated with that target. The `--config <CONFIG>` parameter specifies which configuration to clean, such as Debug, Development, or Shipping. If not specified, all configurations are cleaned. The `--platform <PLATFORM>` parameter specifies which platform to clean, allowing selective cleaning of artifacts for specific platforms. The `--verbose` flag enables detailed output showing which files are being deleted.

**Examples:**

```powershell
# Clean all Debug artifacts
we clean --config Debug

# Clean specific module
we clean --target WERenderer --config Development

# Clean all configurations
we clean
```

### rebuild

The rebuild command performs a clean and build operation in a single step, combining the functionality of the clean and build commands. This is a convenience command that is commonly used when you need to force a complete rebuild of the project or a specific target. The rebuild command first removes the build artifacts for the specified target and then initiates a fresh build, ensuring that all files are recompiled from scratch. This is particularly useful when troubleshooting build issues that may be caused by stale or corrupted build artifacts, or when configuration changes require a complete rebuild to take effect.

```powershell
we rebuild [options]
```

The rebuild command accepts the same parameters as the build command, as it essentially performs a clean followed by a build. The `--target <NAME>` parameter specifies which module or target to rebuild, allowing focused rebuilds of specific components. The `--config <CONFIG>` parameter specifies the build configuration to use for the rebuild. The `--platform <PLATFORM>` parameter specifies the target platform. The `--jobs <N>` parameter controls the number of parallel jobs to use during the build phase. The `--verbose` flag enables detailed output for both the clean and build phases.

**Examples:**

```powershell
# Rebuild entire project
we rebuild --config Development

# Rebuild specific module
we rebuild --target WECore --config Debug
```

### package

The package command packages build artifacts for distribution, creating distributable packages containing the built binaries, libraries, and other required files. This command is typically used when preparing releases or when you need to distribute build artifacts to other machines or users. The packaging process collects the relevant build outputs, organizes them according to the target platform and configuration, and creates a package in a format suitable for distribution. The command can package entire projects or specific targets, allowing flexibility in what gets included in the distribution.

```powershell
we package [options]
```

The package command accepts parameters to control what gets packaged. The `--target <NAME>` parameter allows packaging a specific target rather than the entire project, which is useful when you only need to distribute certain components. The `--config <CONFIG>` parameter specifies which build configuration to package, such as Shipping for release builds. The `--platform <PLATFORM>` parameter specifies which target platform to package for, allowing creation of platform-specific distributions.

**Examples:**

```powershell
# package shipping build
we package --config Shipping
```

## Execution Commands

### run

The run command executes a built target, typically the editor or game executable, launching it with the appropriate configuration and environment. This command is used to run the programs that have been built by the build system, providing a convenient way to launch the editor, game, or other executables without needing to manually locate and execute them. The run command automatically locates the built executable based on the specified target and configuration, sets up the required environment, and launches the program. This ensures that the executable runs with the correct working directory, environment variables, and other settings required for proper operation.

```powershell
we run [options]
```

The run command accepts parameters to control what gets run and how. The `--target <NAME>` parameter specifies which target to run, with Editor being the default. This allows running different executables such as the editor, game, or custom tools. The `--config <CONFIG>` parameter specifies which build configuration to run, with Development being the default. This ensures that the correct version of the executable is launched based on the desired configuration. The `--args <ARGUMENTS>` parameter allows passing command-line arguments to the target executable, which is useful for specifying projects, enabling specific features, or controlling runtime behavior.

**Examples:**

```powershell
# Run editor in Development configuration
we run --target Editor --config Development

# Run with custom arguments
we run --target Editor --config Development --args "-project MyProject"
```

### daemon

The daemon command starts the IgniteBT daemon for background build operations and distributed compilation coordination. The daemon is a background process that provides build services without requiring manual initiation of each build operation. This is particularly useful for distributed builds, where the daemon coordinates work across multiple machines, and for continuous integration scenarios where builds need to run automatically. The daemon listens for build requests, manages worker processes, and handles the complexity of distributed build coordination. Running the daemon can improve build efficiency by keeping build processes warm and ready to execute builds quickly.

```powershell
we daemon [options]
```

The daemon command accepts parameters to control its behavior. The `--port <PORT>` parameter specifies the port number on which the daemon should listen for connections, allowing customization to avoid port conflicts or to integrate with existing infrastructure. The `--workers <N>` parameter specifies the number of worker processes to spawn, controlling the level of parallelism available for background builds. More workers can process more builds concurrently but consume more system resources.

**Examples:**

```powershell
# Start daemon with default settings
we daemon

# Start daemon with custom port
we daemon --port 8080
```

## Project Management Commands

### project

The project command manages project files and project-level operations, providing tools for creating, listing, and opening projects within the workspace. This command is essential for working with multiple projects or for managing the project structure within a larger codebase. The project command handles the complexity of project file management, ensuring that projects are properly configured and that the build system can locate and work with them correctly. Projects in IgniteBT represent logical groupings of work, such as different games, applications, or components that share a common codebase but have distinct build requirements.

```powershell
we project <action> [options]
```

The project command supports several actions for different project management tasks. The `list` action lists all projects in the workspace, providing an overview of available projects and their status. This is useful for understanding what projects are available and their current state. The `open` action opens a specified project, making it the active project for subsequent build operations. This is necessary when working with multiple projects to ensure that build commands operate on the intended project. The `create` action creates a new project with the specified name, setting up the necessary project files and configuration. This automates the project creation process, ensuring that new projects are properly configured from the start.

**Examples:**

```powershell
# List all projects
we project list

# Open specific project
we project open MyProject

# Create new project
we project create NewProject
```

### plugin

The plugin command manages build plugins and extensions, providing tools for listing, building, enabling, and disabling plugins that extend the functionality of the build system. Plugins are modular extensions that can add new commands, modify build behavior, or integrate with external tools and services. The plugin system allows the build system to be customized and extended without modifying the core codebase, making it possible to adapt the build system to specific project requirements or to integrate with organization-specific tools and workflows. Plugins can be developed by the project team or obtained from third-party sources, providing a flexible extension mechanism.

```powershell
we plugin <action> [options]
```

The plugin command supports several actions for plugin management. The `list` action lists all available plugins, showing which plugins are installed and their current status. This provides visibility into what extensions are available in the build system. The `build` action builds a specified plugin, compiling it and making it available for use. This is necessary when developing plugins or when plugins need to be rebuilt after changes. The `enable` action enables a plugin, activating it so that its functionality is available during builds. This allows plugins to be selectively enabled based on project needs. The `disable` action disables a plugin, deactivating it so that its functionality is no longer available. This is useful for troubleshooting or when a plugin is not needed for a particular build.

**Examples:**

```powershell
# List available plugins
we plugin list

# Build specific plugin
we plugin build MyPlugin

# Enable plugin
we plugin enable MyPlugin
```

### modules

The modules command lists and manages build modules, providing visibility into the module structure of the project and the relationships between modules. Modules are the fundamental building blocks of projects in IgniteBT, representing logical groupings of source code that are built together. Understanding the module structure is important for navigating large codebases and for understanding the impact of changes. The modules command can display simple lists of modules, dependency graphs showing how modules depend on each other, or hierarchical trees showing the organization of modules within the project.

```powershell
we modules [options]
```

The modules command accepts parameters to control how module information is displayed. The `--graph` flag displays the module dependency graph, showing how modules depend on each other and the direction of those dependencies. This visualization is invaluable for understanding the impact of changes and for identifying potential circular dependencies. The `--tree` flag displays the module hierarchy, showing how modules are organized within the project structure. This helps with navigating the codebase and understanding the architectural organization of the project.

**Examples:**

```powershell
# List all modules
we modules

# Display dependency graph
we modules --graph

# Display module hierarchy
we modules --tree
```

## SDK Management Commands

### sdk

The sdk command manages and validates development SDKs, providing tools for detecting, listing, and validating the software development kits required for building the project. SDKs are external libraries and tools that the project depends on, such as graphics APIs, windowing systems, and other third-party libraries. Proper SDK management is critical for ensuring that builds are reproducible and that all developers have the required dependencies installed. The sdk command automates the process of detecting installed SDKs, validating their versions against project requirements, and identifying any missing or misconfigured SDKs.

```powershell
we sdk <action> [options]
```

The sdk command supports several actions for SDK management. The `list` action lists all detected SDKs, showing which SDKs have been found, their versions, and their installation paths. This provides a comprehensive overview of the development environment and helps identify what is available. The `detect` action performs a fresh detection of installed SDKs, scanning standard installation locations and updating the SDK database. This is useful when SDKs have been installed or updated since the last detection. The `validate` action validates the SDK configuration, checking that detected SDKs meet the project's version requirements and that all required SDKs are present. This helps catch configuration issues before they cause build failures.

**Examples:**

```powershell
# List all detected SDKs
we sdk list

# Detect installed SDKs
we sdk detect

# Validate SDK configuration
we sdk validate
```

### setup

The setup command configures the development environment and installs the `we` command globally, providing a streamlined way to get started with IgniteBT. This command handles the initial configuration required to use the build system, including setting up the command-line launcher, configuring environment variables, and validating that all prerequisites are met. The setup command is typically run once when first setting up a development environment, but it can also be run to reconfigure or repair the setup if needed. The command provides clear feedback about what it is doing and reports any issues that need to be addressed.

```powershell
we setup [options]
```

The setup command accepts parameters to control the installation behavior. The `--global` flag installs the `we` command globally, adding it to the system PATH so that it can be invoked from any directory without needing to specify the full path. This is the recommended approach for most development setups as it provides the most convenient user experience. The `--path <PATH>` parameter allows specifying a custom installation path for the `we` command, which is useful for scenarios where the default installation location is not appropriate or when you want to maintain multiple versions of the build system.

**Examples:**

```powershell
# Configure development environment
we setup

# Install we command globally
we setup --global
```

## Diagnostic Commands

### doctor

The doctor command performs comprehensive environment and dependency diagnostics, providing a thorough health check of the build environment. This command is invaluable for troubleshooting build issues, validating environment setup, and ensuring that all prerequisites are properly configured. The doctor command checks a wide range of factors including SDK availability and version compatibility, tool installation and configuration, file system permissions, environment variable settings, and dependency integrity. The command provides clear, actionable feedback about any issues it detects, making it easier to identify and resolve problems before they cause build failures.

```powershell
we doctor [options]
```

The doctor command accepts parameters to control the diagnostic process. The `--verbose` flag enables detailed diagnostic output, providing comprehensive information about each check that is performed. This is useful when you need to understand exactly what is being checked and why a particular check is failing. The `--fix` flag attempts to automatically fix detected issues where possible. The doctor command can fix certain common issues automatically, such as correcting environment variables, creating missing directories, or updating configuration files. Not all issues can be automatically fixed, but the command will attempt to resolve what it can and provide guidance for issues that require manual intervention.

**Examples:**

```powershell
# Run diagnostics
we doctor

# Run with detailed output
we doctor --verbose

# Attempt to fix issues
we doctor --fix
```

### version

The version command displays IgniteBT version information, showing the current version of the build system that is installed. This command is useful for verifying which version of the build system is being used, which is important for ensuring compatibility and for troubleshooting issues that may be version-specific. The version information includes both the version number and the product name, providing clear identification of the build system.

```powershell
we version
```

The version command does not accept any parameters and simply outputs the version information to the console. The output format is consistent and machine-parseable, making it easy to use in scripts or automated tools that need to check the build system version.

**Output:**

```
IgniteBT v1.0.0
WindEffects Build Tool
```

### graph

The graph command generates and displays the build dependency graph, providing a visual representation of how different modules and targets depend on each other. The dependency graph is a fundamental data structure in the build system, and visualizing it can be invaluable for understanding the project structure, identifying potential circular dependencies, and understanding the impact of changes. The graph can be displayed in various formats and can be scoped to specific targets or modules, allowing focused analysis of particular parts of the dependency graph.

```powershell
we graph [options]
```

The graph command accepts parameters to control the graph generation and output. The `--output <FILE>` parameter specifies the output file path where the graph should be saved. If not specified, the graph is displayed to the console in a text format. The `--format <FORMAT>` parameter specifies the output format, with supported formats including dot (Graphviz DOT format), svg (Scalable Vector Graphics), and png (Portable Network Graphics). Different formats are useful for different purposes, with DOT being useful for further processing, SVG being ideal for web display, and PNG being suitable for documents and presentations. The `--target <NAME>` parameter specifies that the graph should be generated for a specific target rather than the entire project, allowing focused analysis of particular components.

**Examples:**

```powershell
# Display dependency graph
we graph

# Export to DOT format
we graph --output dependencies.dot --format dot

# Graph specific target
we graph --target WECore
```

### benchmark

The benchmark command runs build performance benchmarks, measuring and analyzing the performance of the build system under various conditions. This command is useful for identifying performance bottlenecks, optimizing build configuration, and understanding how different settings affect build times. The benchmark command can test different levels of parallelism, measure the impact of caching, and compare performance across different configurations. The results provide actionable insights into how to optimize the build process for maximum efficiency.

```powershell
we benchmark [options]
```

The benchmark command accepts parameters to control the benchmark execution. The `--jobs <N>` parameter allows testing with specific job counts, which is useful for determining the optimal level of parallelism for a given system. Multiple job counts can be specified as a comma-separated list to test multiple scenarios in a single run. The `--iterations <N>` parameter specifies the number of benchmark iterations to run, with more iterations providing more statistically significant results but taking longer to complete. The `--target <NAME>` parameter allows benchmarking a specific target rather than the entire project, which is useful for focused performance analysis of particular components.

**Examples:**

```powershell
# Run default benchmarks
we benchmark

# Test with different job counts
we benchmark --jobs 8,16,32

# Benchmark specific target
we benchmark --target WECore
```

## Common Workflows

### Initial Project Setup

The initial project setup workflow prepares a new development environment for building with IgniteBT. This workflow should be followed when setting up a new machine or when starting work on a new project. The setup process begins with configuring the development environment using the setup command, which installs the necessary tooling and configures environment variables. After setup, it is important to validate that all required SDKs are properly installed and configured using the sdk validate command. This ensures that the build system can locate all required dependencies. Finally, an initial build is performed to verify that the environment is correctly configured and that the build system can successfully compile the project.

```powershell
# Configure environment
we setup

# Validate SDKs
we sdk validate

# Initial build
we build --config Development
```

### Development Cycle

The development cycle workflow represents the typical iterative process used during active development. This cycle balances the need for quick iteration with the need to ensure that changes are correctly integrated. The cycle begins with an incremental build, which compiles only the files that have changed or are affected by changes. This is the most common operation during development and provides fast feedback. After a successful build, the editor or other target is launched to test the changes. If issues are encountered or if major changes have been made that require a fresh start, a clean and rebuild can be performed to ensure that all artifacts are rebuilt from scratch.

```powershell
# Incremental build
we build --config Development

# Run editor
we run --target Editor --config Development

# Clean and rebuild if needed
we rebuild --config Debug
```

### Release Preparation

The release preparation workflow is used when preparing a build for distribution or deployment. This workflow ensures that the build is optimized, clean, and properly packaged for distribution. The process begins with cleaning all build artifacts to ensure a fresh start, eliminating any potential issues from stale or corrupted artifacts. A clean build is then performed in the Shipping configuration, which applies maximum optimizations and excludes debug information to produce the smallest and fastest binaries. Increased parallelism is often used for release builds to minimize build time. Finally, the build artifacts are packaged for distribution using the package command, which creates distributable packages containing all necessary files.

```powershell
# Clean all artifacts
we clean

# Build shipping configuration
we build --config Shipping --jobs 16

# Package for distribution
we package --config Shipping
```

### Troubleshooting

The troubleshooting workflow is used when encountering build issues or unexpected behavior. This workflow systematically diagnoses and resolves problems by checking the environment, validating configuration, and performing clean rebuilds. The process begins with running diagnostics using the doctor command with verbose output, which provides comprehensive information about the build environment and any issues that may be present. SDK configuration is then validated to ensure that all required SDKs are properly installed and configured. If issues persist, a clean rebuild with verbose output is performed to eliminate potential issues from stale artifacts and to provide detailed information about the build process for further diagnosis.

```powershell
# Run diagnostics
we doctor --verbose

# Validate SDK configuration
we sdk validate

# Clean and rebuild with verbose output
we rebuild --config Debug --verbose
```

## Exit Codes

IgniteBT uses standardized exit codes to indicate the result of command execution. These exit codes can be used in scripts and automation to determine whether a command succeeded or failed and to take appropriate action based on the type of failure. An exit code of `0` indicates success, meaning that the command completed without errors. An exit code of `1` indicates a general error, which is used for errors that do not fall into more specific categories. An exit code of `2` indicates a build failure, meaning that the compilation or linking process failed. An exit code of `3` indicates a configuration error, meaning that there is a problem with the build system configuration. An exit code of `4` indicates a dependency error, meaning that required dependencies could not be resolved. An exit code of `5` indicates that an SDK was not found, meaning that a required SDK is missing or could not be located.

## Environment Variables

IgniteBT respects several environment variables that allow customization of default behavior without needing to specify options on every command. The `IGNITEBT_CONFIG` environment variable sets the default build configuration, allowing you to specify a preferred configuration such as Development or Shipping that will be used unless explicitly overridden. The `IGNITEBT_PLATFORM` environment variable sets the default target platform, allowing you to specify a preferred platform such as Win64 or Linux. The `IGNITEBT_JOBS` environment variable sets the default parallel job count, allowing you to specify a preferred level of parallelism that will be used unless explicitly overridden. The `IGNITEBT_VERBOSE` environment variable enables verbose output globally when set to `1`, providing detailed output for all commands without needing to specify the `--verbose` flag on each command. The `IGNITEBT_BUILD_ROOT` environment variable sets a custom build directory path, allowing you to specify an alternative location for build artifacts.

## Configuration Files

IgniteBT uses several configuration files to manage different aspects of the build system. The `.engine` file is the project descriptor file that defines the basic structure and configuration of the project, including locations of source code, build output directories, and other fundamental project settings. This file serves as the entry point for the build system. The `.SDKs.json` file contains SDK path configuration, providing explicit paths to SDKs and other dependencies. This file allows developers to override automatic detection or configure non-standard installations. The `bootstrap.manifest` file contains build system bootstrap configuration, providing information needed to bootstrap the build system on first use. This file is automatically generated and maintained by the build system.

## Additional Help

For command-specific help, use the help flag with any command to display detailed usage information. The help system provides comprehensive documentation for each command, including syntax, parameters, examples, and common usage patterns. This is particularly useful when learning new commands or when you need to reference the exact syntax or available options for a command.

```powershell
we <command> --help
```

This displays detailed usage information for the specified command, including all available parameters, their descriptions, and examples of common usage patterns. The help information is automatically generated from the command metadata, ensuring that it is always consistent with the actual command implementation.
