# Getting Started with IgniteBT

This guide walks you through setting up IgniteBT and performing your first build, providing a comprehensive introduction to the build system for WindEffects Engine. IgniteBT is the build system for WindEffects Engine and is automatically included with the engine, requiring no separate installation or download. The guide covers everything from initial environment setup to performing your first build and troubleshooting common issues. By following this guide, you will have a fully configured development environment and a working build of the engine, ready for development work.

## Prerequisites

Before using IgniteBT, ensure your development environment meets these requirements. Having the correct tools and SDKs installed is essential for a smooth build process and to avoid common issues that can arise from missing or misconfigured dependencies. The prerequisites are divided into required software that must be installed for the build system to function and optional software that may be needed depending on the specific project configuration or third-party dependencies.

### Required Software

The .NET 8.0 SDK is required for running IgniteBT, as the build system is built on .NET and relies on the .NET runtime for its operation. The SDK can be downloaded from the official Microsoft website at dotnet.microsoft.com. After installation, you should verify that the SDK is correctly installed by running `dotnet --version` from a command prompt, which should display the installed version number. It is important to add the .NET SDK to your system PATH during installation to ensure that it can be invoked from any directory.

Visual Studio 2022 with the C++ compiler and toolchain is required for compiling C++ code. You should install Visual Studio 2022 with the "Desktop development with C++" workload selected, which includes the MSVC compiler, standard library, and other tools needed for C++ development. It is important to ensure that the latest updates are installed, as these updates include important bug fixes, performance improvements, and compatibility updates that may be required for building the engine. The Visual Studio installer can be used to check for and install updates.

The Vulkan SDK is required for graphics development dependencies, as WindEffects Engine uses Vulkan for graphics rendering. The Vulkan SDK can be downloaded from the LunarG website at vulkan.lunarg.com. During installation, the SDK should be added to your system PATH if this option is not selected automatically. The Vulkan SDK includes the headers, libraries, and tools needed for Vulkan development, and having it correctly installed is essential for building the engine successfully.

Git is required for version control operations, as the engine source code is maintained in a Git repository. Git can be downloaded from git-scm.com and should be added to your system PATH during installation. After installation, verify that Git is correctly installed by running `git --version`, which should display the installed version. Git is used not only for obtaining the source code but also for various build operations that may interact with the repository.

### Optional Software

CMake is required for building some third-party dependencies that the engine relies on. Many third-party libraries use CMake as their build system, and having CMake installed allows these dependencies to be built automatically as part of the engine build process. CMake can be downloaded from the official CMake website and should be added to your system PATH.

Python 3 is required for some build scripts and tools that are used during the build process. Python is a versatile scripting language that is commonly used for build automation, and having Python 3 installed ensures that any Python-based build scripts can run correctly. Python can be downloaded from the official Python website.

Perl is required for building certain third-party libraries that use Perl scripts as part of their build process. While less common than CMake or Python, some libraries still rely on Perl for configuration or build steps. Perl can be downloaded from the official Perl website if needed.

## Initial Setup

### 1. Clone the Repository

The first step in setting up your development environment is to clone the WindEffects Engine repository to your local machine. This will give you access to all the source code, build configuration, and other files needed to build the engine. Use Git to clone the repository from the appropriate source, then navigate into the cloned directory. The repository contains the entire engine source code along with the IgniteBT build system, so cloning it gives you everything you need in one operation.

```powershell
git clone https://github.com/your-org/windeffects.git
cd windeffects
```

### 2. Verify Environment

After cloning the repository, you should verify that your development environment is correctly configured by running the diagnostic command. The doctor command performs a comprehensive check of your environment, verifying that all required tools and SDKs are installed and properly configured. This proactive check helps identify issues before they cause build failures, saving time and frustration. The doctor command checks the .NET SDK installation to ensure that the correct version is installed and accessible, verifies the Visual Studio installation to confirm that the C++ toolchain is available, checks Vulkan SDK availability to ensure graphics dependencies can be built, and validates that other required tools and dependencies are present.

```powershell
we doctor
```

If any issues are detected during the diagnostic check, the doctor command will provide clear guidance on resolving them. This guidance may include information about missing tools, version incompatibilities, or configuration problems. Following this guidance will help ensure that your environment is correctly configured before you attempt to build the engine.

### 3. Configure SDK Paths

If the doctor command reports missing SDKs or if you have SDKs installed in non-standard locations, you will need to configure the SDK paths in the `IgniteBT.SDKs.json` file. This file is located in the root of the repository and allows you to explicitly specify where SDKs are installed. The file uses JSON format and includes sections for SDK paths, additional include paths, and additional library paths. Configuring SDK paths explicitly is useful when automatic detection fails or when you need to use a specific version of an SDK that is not in the standard location.

```json
{
  "SDKPaths": {
    "VulkanSDK": "C:\\VulkanSDK\\1.3.290.0",
    "SDL3": "C:\\SDL3"
  },
  "AdditionalIncludePaths": {},
  "AdditionalLibraryPaths": {}
}
```

After updating the SDK configuration file, you should run the SDK detection and validation commands to ensure that the build system can now locate the SDKs correctly.

### 4. Install the `we` Command Globally (Optional)

For convenient access from any directory, you can install the `we` command globally on your system. This step is optional but recommended, as it allows you to run build commands from any directory without needing to specify the full path to the build system launcher. Global installation adds the `we` command to your system PATH, making it available system-wide just like other command-line tools. This is particularly convenient if you work with multiple projects or if you frequently switch between different directories.

```powershell
we setup --global
```

The global installation process modifies your system PATH to include the directory containing the `we` command launcher. After installation, you may need to restart your terminal or command prompt for the PATH changes to take effect. Once installed, you can run `we` commands from any directory, which significantly improves the user experience and reduces the friction of using the build system.

## Your First Build

### Building in Development Configuration

The Development configuration provides optimized builds with debugging symbols, making it ideal for active development work. This configuration strikes a balance between performance and debuggability, applying compiler optimizations while still including debugging symbols. This means that the built code runs reasonably fast for testing and iteration, but you can still debug issues when they arise. The Development configuration is the recommended default for day-to-day development work, as it provides the best compromise between build time, runtime performance, and debugging capability.

```powershell
we build --config Development
```

The first build will take longer than subsequent builds, as there are several one-time setup tasks that need to be completed. The first build compiles IgniteBT itself, which is the build system that will be used for all subsequent builds. It also downloads and builds third-party dependencies that the engine relies on, which can include graphics libraries, math libraries, and other external components. The build compiles all engine modules, which are the various components that make up the engine such as the core systems, renderer, and tools. Finally, it generates required artifacts such as export definition files and other generated code. All of these one-time tasks mean that the first build can take significantly longer than later builds, but this investment pays off in faster subsequent builds.

Subsequent builds will be significantly faster due to incremental building and caching. The build system tracks which files have changed and only recompiles what is necessary, avoiding redundant work. The caching system stores build artifacts and dependency information, allowing the system to reuse work from previous builds. This means that after the initial build, making small changes and rebuilding will typically complete much faster, often in seconds rather than minutes.

### Build Output

Build artifacts are organized in the `Build` directory, which is created automatically the first time you build. The Build directory contains all the outputs of the build process, organized in a logical structure that separates different types of artifacts and different build configurations. This organization makes it easy to locate specific outputs and to manage the build directory without conflicts between different configurations or platforms.

```
Build/
├── Output/
│   └── Development/
│       └── Win64/
│           ├── WECore.dll
│           ├── WERenderer.dll
│           └── Editor.exe
├── Intermediate/
│   └── Development/
│       └── Win64/
│           └── [Object files and PDBs]
└── Logs/
    └── IgniteBT-[date].log
```

The Output directory contains the final binaries and libraries that are the primary products of the build process. These are the files that you actually use, such as executable programs, shared libraries, and static libraries. The Output directory is organized by configuration and platform, so artifacts from different builds are kept separate. The Intermediate directory contains object files and compilation artifacts that are intermediate products of the compilation process. These files are not typically used directly by developers but are essential for incremental builds, as they allow the build system to avoid recompiling source files that have not changed. The Logs directory contains build and runtime logs, providing a record of build operations that can be invaluable for troubleshooting and analysis.

### Running the Editor

After a successful build, you can launch the editor to begin working with the engine. The run command automatically locates the built executable based on the specified target and configuration, sets up the required environment, and launches the program. This ensures that the executable runs with the correct working directory, environment variables, and other settings required for proper operation. The run command is the recommended way to launch the editor, as it handles all the necessary setup automatically.

```powershell
we run --target Editor --config Development
```

## Common Build Configurations

IgniteBT supports three main configurations, each optimized for different scenarios and stages of development. Understanding these configurations and when to use each one is important for optimizing your development workflow and ensuring that you have the right balance of performance, debugging capability, and build time for your current needs.

### Debug

The Debug configuration provides full debugging information with no compiler optimizations, making it ideal for debugging complex issues that require detailed inspection of program state. This configuration includes maximum debugging information, allowing you to inspect variables, step through code line by line, and analyze program execution in detail. Compiler optimizations are disabled to ensure that the generated code matches the source code as closely as possible, which is important for accurate debugging. However, this comes at the cost of slow execution speed and large binary size, as the lack of optimizations and the inclusion of extensive debugging information significantly impact performance and file size. The Debug configuration is typically used only when actively debugging difficult issues, as the performance penalty makes it impractical for regular development work.

```powershell
we build --config Debug
```

The characteristics of the Debug configuration include maximum debugging information for comprehensive inspection capabilities, no compiler optimizations to ensure code behavior matches source code structure, slowest execution due to lack of optimizations and runtime checks, and largest binary size due to embedded debugging symbols and metadata.

### Development

The Development configuration provides an optimized build with debugging symbols, making it the recommended choice for daily development work. This configuration strikes an optimal balance between performance and debuggability, applying compiler optimizations to improve runtime performance while still including debugging symbols for debugging when needed. The inclusion of debugging symbols means that you can still attach a debugger and inspect program state, but the optimizations mean that the code runs faster and the binaries are smaller than in the Debug configuration. This configuration provides good performance for testing and iteration while still maintaining reasonable debugging capabilities. The Development configuration is the default for most development work, as it provides the best compromise between build time, runtime performance, and debugging capability.

```powershell
we build --config Development
```

The characteristics of the Development configuration include debugging symbols being included for debugging capability when needed, compiler optimizations being enabled to improve runtime performance, good performance that allows for interactive testing and iteration, and reasonable binary size that balances debugging information with optimization.

### Shipping

The Shipping configuration provides a fully optimized release build, making it suitable for final releases and distribution. This configuration excludes debugging information entirely and applies maximum compiler optimizations to produce the smallest and fastest possible binaries. The absence of debugging information reduces binary size and eliminates the possibility of debugging the release build, but this is appropriate for shipped software where debugging is not needed. The maximum optimizations provide the best possible runtime performance, which is important for user experience. The Shipping configuration should be used for final builds that will be distributed to users, as it provides the optimal combination of small size and fast execution.

```powershell
we build --config Shipping
```

The characteristics of the Shipping configuration include no debugging information to minimize binary size and prevent debugging, maximum optimizations for the best possible runtime performance, best performance among all configurations for optimal user experience, and smallest binary size for efficient distribution and reduced storage requirements.

## Building Specific Modules

For faster iteration during development, you can build specific modules rather than the entire project. This is particularly useful when you are working on a specific component and want to quickly see the results of your changes without waiting for the entire project to rebuild. Building specific modules significantly reduces build time by focusing compilation on only the parts of the project that you are actively working on. The build system automatically handles dependencies, so if the module you are building depends on other modules, those dependencies will be built as needed. This targeted approach to building can dramatically improve iteration speed, especially in large projects with many modules.

```powershell
# Build only the core module
we build --target WECore --config Development

# Build only the renderer
we build --target WERenderer --config Development
```

To see which modules are available in the project, you can use the modules command to list all discovered modules. This command shows the module names and their relationships, helping you understand the module structure and identify which module you want to build.

```powershell
we modules
```

## Cleaning Build Artifacts

Cleaning build artifacts removes the intermediate and output files produced by previous builds, forcing a complete rebuild on the next build. This is useful when you need to eliminate potential issues caused by stale or corrupted build artifacts, or when configuration changes require a fresh start. The clean operation is selective based on the parameters provided, allowing you to clean specific configurations, targets, or platforms without affecting other build artifacts. This selective cleaning is useful when you want to force a rebuild of only certain parts of the project while preserving the artifacts for other parts.

```powershell
# Clean specific configuration
we clean --config Debug

# Clean specific module
we clean --target WECore --config Development

# Clean all artifacts
we clean
```

The clean command removes files from the build directory but does not affect source code or configuration files, making it safe to run at any time. The command removes object files, compiled binaries, and other build artifacts, but leaves your source code untouched. After cleaning, the next build will recompile everything from scratch, which takes longer than an incremental build but ensures a completely fresh start.

## Rebuilding

The rebuild command combines cleaning and building into a single operation, providing a convenient way to force a complete rebuild without needing to run two separate commands. This is particularly useful when troubleshooting build issues that may be caused by stale or corrupted build artifacts, or when configuration changes require a complete rebuild to take effect. The rebuild command first removes the build artifacts for the specified target and then initiates a fresh build, ensuring that all files are recompiled from scratch. This two-step process ensures that no stale artifacts from previous builds can interfere with the new build.

```powershell
# Rebuild entire project
we rebuild --config Development

# Rebuild specific module
we rebuild --target WECore --config Debug
```

The rebuild command accepts the same parameters as the build command, allowing you to rebuild specific targets or configurations. This flexibility means you can rebuild just what you need without having to rebuild the entire project if not necessary.

## Parallel Builds

Controlling the number of parallel jobs allows you to tune the build process for your specific hardware and build requirements. Modern multi-core processors can execute multiple compilation tasks simultaneously, and IgniteBT can take advantage of this by running multiple jobs in parallel. The optimal number of parallel jobs depends on factors such as the number of CPU cores, available memory, and the nature of the build. Using more parallel jobs can reduce build time by utilizing more CPU cores, but using too many can actually slow down the build due to resource contention and overhead.

```powershell
# Use 8 parallel jobs
we build --config Development --jobs 8

# Use all available cores
we build --config Development --jobs 0
```

When you specify a job count of zero, IgniteBT automatically determines the optimal number of jobs based on your system's hardware capabilities. This automatic detection typically uses the number of logical CPU cores as a starting point and may adjust based on available memory and other factors. For most systems, letting IgniteBT determine the optimal job count provides good performance without manual tuning.

## Verbose Output

Enabling verbose output provides detailed information about the build process, which is invaluable for troubleshooting and understanding what the build system is doing. Verbose output shows the exact compiler commands being executed, file-by-file compilation progress, dependency resolution details, and linker output. This level of detail can help you identify which files are being rebuilt, why they are being rebuilt, and what errors or warnings are being generated. Verbose output is particularly useful when diagnosing build failures or when trying to understand the impact of changes on the build process.

```powershell
we build --config Development --verbose
```

When verbose output is enabled, the build system displays detailed compiler commands showing all flags and parameters being passed to the compiler. It shows file-by-file compilation progress so you can see exactly which files are being compiled and in what order. Dependency resolution details show how the build system is determining which files need to be rebuilt based on changes. Linker output shows the linking process and any linker warnings or errors. This comprehensive information makes it much easier to understand what is happening during the build and to identify the root cause of any issues.

## Troubleshooting

### Build Failures

Build failures can occur for various reasons, from missing dependencies to configuration errors to compiler errors in the source code. When a build fails, it is important to systematically diagnose the issue to identify the root cause and determine the appropriate resolution. The first step is to check the build log, which contains detailed information about what went wrong during the build process. The build log is located in the `Build/Logs` directory and is named with the date of the build. Examining this log can reveal error messages, warnings, and other clues about what caused the failure.

If the build log does not provide enough information to resolve the issue, running diagnostics with verbose output can provide additional context. The doctor command performs a comprehensive check of the build environment and can identify configuration problems, missing dependencies, or other issues that may be contributing to the build failure. Running the doctor command with the verbose flag provides detailed information about each check that is performed.

If the issue appears to be related to stale or corrupted build artifacts, performing a clean rebuild can often resolve the problem. A clean rebuild removes all build artifacts and rebuilds everything from scratch, eliminating any potential issues from corrupted intermediate files. Running the clean rebuild with verbose output provides detailed information about the rebuild process, which can help identify if the issue persists after cleaning.

Finally, verifying the SDK configuration ensures that all required SDKs are properly installed and configured. SDK configuration issues are a common cause of build failures, particularly for graphics-related builds that depend on the Vulkan SDK. The sdk validate command checks that all required SDKs are present and meet the project's version requirements.

1. Check the build log in `Build/Logs/IgniteBT-[date].log`
2. Run diagnostics: `we doctor --verbose`
3. Try a clean rebuild: `we rebuild --config Debug --verbose`
4. Verify SDK configuration: `we sdk validate`

### Missing Dependencies

Missing dependencies can occur when third-party libraries fail to build or when required tools are not installed. Many third-party dependencies are downloaded and built automatically as part of the engine build process, and failures in this process can prevent the overall build from succeeding. If third-party dependencies fail to build, the first step is to check internet connectivity, as some dependencies are downloaded from remote repositories during the build process. A lack of internet connectivity or network issues can prevent these downloads from succeeding.

Verifying that required tools such as CMake, Perl, and Python are installed is also important, as these tools are often needed to build third-party libraries. Even though these tools are listed as optional in the prerequisites, specific third-party dependencies may require them. Checking the build logs in the `Build/Logs` directory can provide detailed error messages from the third-party build processes, which can reveal what specifically went wrong.

If the issue appears to be related to a corrupted or incomplete third-party build, manually cleaning the `Build/Intermediate/ThirdParty` directory and retrying the build can resolve the issue. This forces the third-party dependencies to be rebuilt from scratch, eliminating any potential corruption from a previous build attempt.

1. Check internet connectivity (some dependencies are downloaded)
2. Verify required tools (CMake, Perl, Python) are installed
3. Check `Build/Logs` for detailed error messages
4. Manually clean `Build/Intermediate/ThirdParty` and retry

### SDK Detection Issues

SDK detection issues occur when the build system cannot locate required SDKs, even though they may be installed on the system. This can happen when SDKs are installed in non-standard locations that the automatic detection does not check, or when SDK installation paths have changed. If SDKs are not detected, the first step is to verify the actual SDK installation paths by checking where the SDKs are installed on your system. This information can then be used to update the SDK configuration.

Updating the `IgniteBT.SDKs.json` file with the correct SDK paths allows you to explicitly tell the build system where to find the SDKs. This file is located in the root of the repository and uses JSON format to specify SDK paths. After updating the configuration, running the sdk detect command refreshes the detection process, causing the build system to scan for SDKs again using the updated configuration.

Finally, running the sdk validate command verifies that the SDK configuration is correct and that all required SDKs can now be located. This validation step confirms that the configuration changes have resolved the detection issue.

1. Verify SDK installation paths
2. Update `IgniteBT.SDKs.json` with correct paths
3. Run `we sdk detect` to refresh detection
4. Run `we sdk validate` to verify configuration

### Out-of-Memory Errors

Out-of-memory errors can occur when building very large projects, particularly when using high levels of parallelism. The build process, especially linking, can consume significant amounts of memory, and systems with limited RAM may encounter memory pressure during builds. If you encounter out-of-memory errors, there are several strategies to address the issue.

Reducing the parallel job count is often the most effective solution, as it reduces the number of concurrent compilation tasks and thus reduces peak memory usage. Setting the job count to a lower number such as four can significantly reduce memory consumption while still providing some parallelism. Closing other memory-intensive applications before building can also free up memory for the build process.

Increasing the system page file size provides more virtual memory, which can help when physical RAM is insufficient. The page file acts as an extension of physical memory, allowing the system to swap less frequently used data to disk when memory pressure is high. Finally, ensuring that you are using the 64-bit build tools, which is the default, is important as 64-bit tools can address more memory than 32-bit tools.

1. Reduce parallel job count: `--jobs 4`
2. Close other memory-intensive applications
3. Increase system page file size
4. Use the 64-bit build tools (default)

## Next Steps

After completing your first build and verifying that everything works correctly, there are several resources available to help you deepen your understanding of IgniteBT and become more proficient with the build system. The Command Reference documentation provides detailed information about all available commands, their parameters, and usage examples. This is an essential resource for learning about advanced features and capabilities that you may not have encountered in the getting started guide.

The Architecture Documentation offers an in-depth look at the system design, component organization, and the interactions between different subsystems. Understanding the architecture is valuable for advanced users who need to understand how the build system works internally or who are planning to extend or modify its functionality. This documentation explains the layered architecture, the responsibilities of each layer, and the design principles that guide the system.

The Configuration Guide covers SDK configuration, project setup, environment variables, and advanced tuning options. This guide is essential for setting up the development environment correctly and for customizing the build system behavior to match specific project requirements. Understanding configuration options allows you to optimize the build system for your specific hardware and workflow.

- Read the [Command Reference](./commands.md) for advanced usage
- Review the [Architecture Documentation](./architecture.md) to understand the system
- Configure [SDKs and project settings](./configuration.md)
- Explore [module development](./modules.md) for custom build rules

## Performance Tips

### Optimize Build Times

Optimizing build times is essential for maintaining productivity during development. Several strategies can significantly reduce the time spent waiting for builds to complete. Using the Development configuration rather than Debug provides faster builds while still including debugging symbols, making it the optimal choice for most development work. The Development configuration applies optimizations that improve both build time and runtime performance while still allowing debugging when needed.

Enabling parallel builds allows the build system to utilize multiple CPU cores simultaneously, dramatically reducing build time on multi-core systems. Setting the job count to match your CPU core count provides optimal parallelism without causing resource contention. Building specific modules rather than the entire project focuses compilation on only the parts you are actively working on, which is especially useful for large projects where rebuilding everything would take a long time.

Using incremental builds by avoiding unnecessary clean operations allows the build system to reuse work from previous builds. Only clean when necessary, such as when troubleshooting issues that may be caused by stale artifacts. Keeping the build directory on fast SSD storage rather than slower mechanical hard drives can significantly improve build performance, as build operations are heavily I/O bound.

1. **Use Development Configuration**: Faster than Debug, more useful than Shipping
2. **Enable Parallel Builds**: Use `--jobs` to match your CPU core count
3. **Build Specific Modules**: Only build what you're working on
4. **Use Incremental Builds**: Avoid `clean` unless necessary
5. **SSD Storage**: Keep build directory on fast storage

### Cache Management

IgniteBT maintains a build cache to speed up incremental builds, storing compiled artifacts and dependency information to avoid redundant work. The cache is stored in the `Build/Cache` directory and is a critical component of the build system's performance. The cache is content-addressable, meaning that artifacts are stored and retrieved based on cryptographic hashes of their inputs, ensuring cache correctness even in complex scenarios.

If you experience cache-related issues such as incorrect artifacts being reused or cache corruption, clearing the cache can resolve the problem. Clearing the cache forces the build system to recompute all artifacts from scratch, eliminating any potential issues from corrupted cache entries. After clearing the cache, the next build will take longer as it rebuilds everything, but subsequent builds will benefit from a fresh cache.

```powershell
# Clear the cache
we clean --cache
```

## Continuous Integration

For CI/CD pipelines, using recommended practices ensures reliable and efficient automated builds. Continuous integration environments have different requirements than interactive development, as they prioritize reproducibility and reliability over iteration speed. A typical CI build script should begin by running diagnostics to verify that the build environment is correctly configured, followed by SDK validation to ensure all required dependencies are available.

Cleaning the build configuration ensures a fresh start for each CI build, eliminating any potential issues from stale artifacts. Building with maximum parallelism and verbose output allows the build to complete as quickly as possible while providing detailed logs for troubleshooting if issues occur. Finally, packaging the build artifacts prepares them for distribution or deployment.

```powershell
# CI build script example
we doctor
we sdk validate
we clean --config Shipping
we build --config Shipping --jobs 0 --verbose
we package --config Shipping
```

## Getting Help

If you encounter issues not covered in this guide, there are several resources available to help you resolve them. The build logs in the `Build/Logs` directory contain detailed information about build operations and are often the first place to look when troubleshooting build failures. The logs include error messages, warnings, and timing information that can help identify the root cause of issues.

Running the doctor command with verbose output provides comprehensive diagnostics of the build environment, checking for configuration problems, missing dependencies, and other common issues. The Command Reference documentation provides detailed information about all available commands and their parameters, which can help you understand the full capabilities of the build system and find solutions to specific problems. If all else fails, consulting the main WindEffects Engine documentation may provide additional context and guidance.

1. Check the build logs in `Build/Logs/`
2. Run `we doctor --verbose` for diagnostics
3. Review the [Command Reference](./commands.md)
4. Consult the main WindEffects Engine documentation

## Environment Variables

Configuring default behavior with environment variables allows you to customize the build system without needing to specify options on every command. Environment variables are particularly useful for setting up consistent defaults across your development environment or for configuring CI/CD pipelines. The `IGNITEBT_CONFIG` environment variable sets the default build configuration, allowing you to specify a preferred configuration such as Development or Shipping that will be used unless explicitly overridden on the command line.

The `IGNITEBT_JOBS` environment variable sets the default parallel job count, allowing you to specify a preferred level of parallelism that will be used for builds unless overridden. This is useful for setting a default that matches your system's capabilities. The `IGNITEBT_VERBOSE` environment variable enables verbose output globally when set to `1`, providing detailed output for all commands without needing to specify the `--verbose` flag on each command. This is particularly useful for troubleshooting or when you want detailed information about all build operations.

```powershell
# Set default configuration
set IGNITEBT_CONFIG=Development

# Set default parallel job count
set IGNITEBT_JOBS=8

# Enable verbose output globally
set IGNITEBT_VERBOSE=1
```

## Uninstalling

If you need to remove IgniteBT from your system, the process involves removing the build artifacts, the global command installation, and any environment variables that were configured. To remove the build directory, you can use the Remove-Item command with the Recurse flag to delete the entire Build directory and all its contents. This removes all build artifacts, cache data, and generated files.

If you installed the `we` command globally, you will need to remove it from your system PATH. This typically involves removing the directory that was added to the PATH during global installation. The specific steps depend on how the global installation was performed and your operating system configuration. Finally, if you configured any environment variables such as `IGNITEBT_CONFIG` or `IGNITEBT_JOBS`, you should delete these to clean up your environment configuration.

1. Delete the build directory: `Remove-Item -Recurse Build`
2. Remove global `we` command (if installed globally)
3. Delete environment variables (if configured)

Congratulations! You've successfully set up IgniteBT and completed your first build. You're now ready to develop with WindEffects Engine.
