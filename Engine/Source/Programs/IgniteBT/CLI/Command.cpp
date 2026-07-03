#include "CLI/Command.hpp"
#include "Logging/Logger.hpp"
#include "Configuration/Configuration.hpp"
#include "Platform/Platform.hpp"
#include <sstream>
#include <algorithm>

namespace IgniteBT {

// CommandContext
bool CommandContext::HasFlag(const std::string& flag) const {
    return flags.find(flag) != flags.end();
}

std::string CommandContext::GetFlagValue(const std::string& flag, const std::string& defaultValue) const {
    auto it = flags.find(flag);
    if (it != flags.end()) {
        return it->second;
    }
    return defaultValue;
}

// CommandRegistry
CommandRegistry& CommandRegistry::Get() {
    static CommandRegistry instance;
    return instance;
}

void CommandRegistry::RegisterCommand(std::shared_ptr<ICommand> command) {
    m_commands[command->GetName()] = command;
    LOG_DEBUG("Registered command: " + command->GetName(), "CLI");
}

void CommandRegistry::UnregisterCommand(const std::string& name) {
    m_commands.erase(name);
    LOG_DEBUG("Unregistered command: " + name, "CLI");
}

std::shared_ptr<ICommand> CommandRegistry::GetCommand(const std::string& name) const {
    auto it = m_commands.find(name);
    if (it != m_commands.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<ICommand>> CommandRegistry::GetAllCommands() const {
    std::vector<std::shared_ptr<ICommand>> commands;
    for (const auto& [name, command] : m_commands) {
        commands.push_back(command);
    }
    return commands;
}

Result<void> CommandRegistry::ExecuteCommand(const CommandContext& context) {
    auto command = GetCommand(context.command);
    if (!command) {
        return Result<void>::Failure("Unknown command: " + context.command);
    }
    
    LOG_INFO("Executing command: " + context.command, "CLI");
    return command->Execute(context);
}

// CommandParser
CommandContext CommandParser::Parse(const std::string& commandLine) {
    CommandContext context;
    
    std::istringstream iss(commandLine);
    std::string token;
    
    // First token is the command
    if (iss >> token) {
        context.command = token;
    }
    
    // Parse remaining tokens
    while (iss >> token) {
        if (IsFlag(token)) {
            auto [flag, value] = ParseFlag(token);
            context.flags[flag] = value;
        } else {
            context.arguments.push_back(token);
        }
    }
    
    return context;
}

CommandContext CommandParser::Parse(int argc, char* argv[]) {
    CommandContext context;
    
    if (argc > 0) {
        context.command = argv[0];
    }
    
    for (int i = 1; i < argc; ++i) {
        std::string token = argv[i];
        if (IsFlag(token)) {
            auto [flag, value] = ParseFlag(token);
            context.flags[flag] = value;
        } else {
            context.arguments.push_back(token);
        }
    }
    
    return context;
}

bool CommandParser::IsFlag(const std::string& arg) {
    return arg.starts_with("--") || arg.starts_with("-");
}

std::pair<std::string, std::string> CommandParser::ParseFlag(const std::string& arg) {
    size_t equalsPos = arg.find('=');
    
    if (equalsPos != std::string::npos) {
        // --flag=value format
        std::string flag = arg.substr(0, equalsPos);
        std::string value = arg.substr(equalsPos + 1);
        
        // Remove dashes from flag name
        if (flag.starts_with("--")) {
            flag = flag.substr(2);
        } else if (flag.starts_with("-")) {
            flag = flag.substr(1);
        }
        
        return {flag, value};
    } else {
        // --flag format (boolean flag)
        std::string flag = arg;
        if (flag.starts_with("--")) {
            flag = flag.substr(2);
        } else if (flag.starts_with("-")) {
            flag = flag.substr(1);
        }
        
        return {flag, "true"};
    }
}

// HelpCommand
Result<void> HelpCommand::Execute(const CommandContext& context) {
    auto& registry = CommandRegistry::Get();
    
    if (context.arguments.empty()) {
        // Show general help
        std::cout << "IgniteBT - Ignite Build Tool" << std::endl;
        std::cout << "==========================" << std::endl;
        std::cout << std::endl;
        std::cout << "Available commands:" << std::endl;
        
        auto commands = registry.GetAllCommands();
        for (const auto& command : commands) {
            std::cout << "  " << command->GetName() << " - " << command->GetDescription() << std::endl;
        }
        
        std::cout << std::endl;
        std::cout << "Use 'help <command>' for detailed information about a command." << std::endl;
    } else {
        // Show help for specific command
        std::string commandName = context.arguments[0];
        auto command = registry.GetCommand(commandName);
        
        if (!command) {
            return Result<void>::Failure("Unknown command: " + commandName);
        }
        
        std::cout << "Command: " << command->GetName() << std::endl;
        std::cout << "Description: " << command->GetDescription() << std::endl;
        std::cout << "Usage: " << command->GetUsage() << std::endl;
    }
    
    return Result<void>::Success();
}

// VersionCommand
Result<void> VersionCommand::Execute(const CommandContext& context) {
    (void)context;
    
    Version version{1, 0, 0};
    std::cout << "IgniteBT Version: " << version.ToString() << std::endl;
    std::cout << "Build Configuration: " << ConfigurationManager::Get().GetBuildConfig().outputDirectory << std::endl;
    
    auto& platformManager = PlatformManager::Get();
    std::cout << "Platform: ";
    if (platformManager.IsWindows()) std::cout << "Windows";
    else if (platformManager.IsLinux()) std::cout << "Linux";
    else if (platformManager.IsMac()) std::cout << "Mac";
    else std::cout << "Unknown";
    std::cout << std::endl;
    
    return Result<void>::Success();
}

// CleanCommand
Result<void> CleanCommand::Execute(const CommandContext& context) {
    std::string target = context.GetFlagValue("target");
    bool cleanAll = context.HasFlag("all");
    
    auto* platform = PlatformManager::Get().GetPlatform();
    auto& config = ConfigurationManager::Get();
    Path buildDir = config.GetEngineRoot() / config.GetBuildConfig().outputDirectory;
    
    if (cleanAll) {
        LOG_INFO("Cleaning entire build directory: " + buildDir.string(), "Clean");
        if (platform && platform->DirectoryExists(buildDir)) {
            platform->DeleteDirectory(buildDir, true);
        }
    } else if (!target.empty()) {
        LOG_INFO("Cleaning target: " + target, "Clean");
        // TODO: Implement target-specific cleaning
    } else {
        LOG_WARNING("No target specified, use --all to clean everything", "Clean");
    }
    
    return Result<void>::Success();
}

// RebuildCommand
Result<void> RebuildCommand::Execute(const CommandContext& context) {
    if (context.arguments.empty()) {
        return Result<void>::Failure("No target specified for rebuild");
    }
    
    std::string target = context.arguments[0];
    LOG_INFO("Rebuilding target: " + target, "Rebuild");
    
    // First clean the target
    CommandContext cleanContext;
    cleanContext.command = "clean";
    cleanContext.flags["target"] = target;
    
    auto& registry = CommandRegistry::Get();
    auto cleanCommand = registry.GetCommand("clean");
    if (cleanCommand) {
        cleanCommand->Execute(cleanContext);
    }
    
    // Then build the target
    CommandContext buildContext;
    buildContext.command = "build";
    buildContext.arguments.push_back(target);
    
    auto buildCommand = registry.GetCommand("build");
    if (buildCommand) {
        return buildCommand->Execute(buildContext);
    }
    
    return Result<void>::Failure("Build command not available");
}

// DoctorCommand
Result<void> DoctorCommand::Execute(const CommandContext& context) {
    (void)context;
    
    std::cout << "=== IgniteBT Environment Diagnostics ===" << std::endl;
    
    auto& platformManager = PlatformManager::Get();
    auto* platform = platformManager.GetPlatform();
    auto& config = ConfigurationManager::Get();
    
    // Platform info
    std::cout << "Platform: ";
    if (platformManager.IsWindows()) std::cout << "Windows";
    else if (platformManager.IsLinux()) std::cout << "Linux";
    else if (platformManager.IsMac()) std::cout << "Mac";
    else std::cout << "Unknown";
    std::cout << std::endl;
    
    // Architecture
    std::cout << "Architecture: ";
    if (platformManager.Is64Bit()) std::cout << "64-bit";
    else std::cout << "32-bit";
    std::cout << std::endl;
    
    // Engine root
    std::cout << "Engine Root: " << config.GetEngineRoot().string() << std::endl;
    
    // Build directory
    Path buildDir = config.GetEngineRoot() / config.GetBuildConfig().outputDirectory;
    std::cout << "Build Directory: " << buildDir.string() << std::endl;
    std::cout << "Build Directory Exists: " << (platform && platform->DirectoryExists(buildDir) ? "Yes" : "No") << std::endl;
    
    // Configuration
    std::cout << "Build Configuration: ";
    switch (config.GetBuildConfig().configuration) {
        case BuildConfiguration::Debug: std::cout << "Debug"; break;
        case BuildConfiguration::Development: std::cout << "Development"; break;
        case BuildConfiguration::Profile: std::cout << "Profile"; break;
        case BuildConfiguration::Shipping: std::cout << "Shipping"; break;
        default: std::cout << "Custom"; break;
    }
    std::cout << std::endl;
    
    // Compiler
    std::cout << "Compiler: ";
    switch (config.GetBuildConfig().compiler) {
        case CompilerType::MSVC: std::cout << "MSVC"; break;
        case CompilerType::GCC: std::cout << "GCC"; break;
        case CompilerType::Clang: std::cout << "Clang"; break;
        case CompilerType::AppleClang: std::cout << "AppleClang"; break;
        default: std::cout << "Unknown"; break;
    }
    std::cout << std::endl;
    
    // Features
    std::cout << "Unity Build: " << (config.GetBuildConfig().enableUnityBuild ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Cache: " << (config.GetBuildConfig().enableCache ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Parallel Build: " << (config.GetBuildConfig().enableParallelBuild ? "Enabled" : "Disabled") << std::endl;
    std::cout << "PCH: " << (config.GetBuildConfig().enablePCH ? "Enabled" : "Disabled") << std::endl;
    
    std::cout << "=== Diagnostics Complete ===" << std::endl;
    
    return Result<void>::Success();
}

// BenchmarkCommand
Result<void> BenchmarkCommand::Execute(const CommandContext& context) {
    int iterations = 1;
    
    std::string iterStr = context.GetFlagValue("iterations");
    if (!iterStr.empty()) {
        iterations = std::stoi(iterStr);
    }
    
    LOG_INFO("Running benchmark with " + std::to_string(iterations) + " iteration(s)", "Benchmark");
    
    // TODO: Implement actual benchmarking logic
    std::cout << "Benchmark functionality not yet implemented" << std::endl;
    
    return Result<void>::Success();
}

// PluginsCommand
Result<void> PluginsCommand::Execute(const CommandContext& context) {
    bool list = context.HasFlag("list");
    std::string info = context.GetFlagValue("info");
    
    auto& config = ConfigurationManager::Get();
    
    if (list || (!info.empty())) {
        auto plugins = config.GetAllPluginConfigs();
        
        if (plugins.empty()) {
            std::cout << "No plugins found." << std::endl;
            return Result<void>::Success();
        }
        
        if (!info.empty()) {
            // Show specific plugin info
            auto result = config.GetPluginConfig(info);
            if (result.IsFailure()) {
                return Result<void>::Failure(result.GetError());
            }
            
            auto plugin = result.GetValue();
            std::cout << "Plugin: " << plugin.name << std::endl;
            std::cout << "Version: " << plugin.version << std::endl;
            std::cout << "Description: " << plugin.description << std::endl;
            std::cout << "Dependencies: ";
            for (const auto& dep : plugin.dependencies) {
                std::cout << dep << " ";
            }
            std::cout << std::endl;
        } else {
            // List all plugins
            std::cout << "Available Plugins:" << std::endl;
            for (const auto& plugin : plugins) {
                std::cout << "  " << plugin.name << " v" << plugin.version << " - " << plugin.description << std::endl;
            }
        }
    } else {
        std::cout << "Use --list to list plugins or --info=<plugin> for detailed information." << std::endl;
    }
    
    return Result<void>::Success();
}

// BuildCommand (placeholder)
Result<void> BuildCommand::Execute(const CommandContext& context) {
    if (context.arguments.empty()) {
        return Result<void>::Failure("No target specified for build");
    }
    
    std::string target = context.arguments[0];
    LOG_INFO("Building target: " + target, "Build");
    
    // TODO: Implement actual build logic with BuildGraph
    std::cout << "Build functionality requires BuildGraph implementation" << std::endl;
    
    return Result<void>::Success();
}

// GenerateCommand (placeholder)
Result<void> GenerateCommand::Execute(const CommandContext& context) {
    if (context.arguments.empty()) {
        return Result<void>::Failure("No format specified for generation");
    }
    
    std::string format = context.arguments[0];
    LOG_INFO("Generating project files: " + format, "Generate");
    
    // TODO: Implement actual generation logic with ProjectGenerator
    std::cout << "Project generation requires ProjectGenerator implementation" << std::endl;
    
    return Result<void>::Success();
}

// PackageCommand (placeholder)
Result<void> PackageCommand::Execute(const CommandContext& context) {
    if (context.arguments.empty()) {
        return Result<void>::Failure("No target specified for packaging");
    }
    
    std::string target = context.arguments[0];
    LOG_INFO("Packaging target: " + target, "Package");
    
    // TODO: Implement actual packaging logic with PackageManager
    std::cout << "Packaging functionality requires PackageManager implementation" << std::endl;
    
    return Result<void>::Success();
}

// TestCommand (placeholder)
Result<void> TestCommand::Execute(const CommandContext& context) {
    std::string filter = context.GetFlagValue("filter");
    
    LOG_INFO("Running tests", "Test");
    if (!filter.empty()) {
        LOG_INFO("Filter: " + filter, "Test");
    }
    
    // TODO: Implement actual test logic
    std::cout << "Test functionality not yet implemented" << std::endl;
    
    return Result<void>::Success();
}

// GraphCommand (placeholder)
Result<void> GraphCommand::Execute(const CommandContext& context) {
    if (context.arguments.empty()) {
        return Result<void>::Failure("No target specified for graph");
    }
    
    std::string target = context.arguments[0];
    std::string output = context.GetFlagValue("output");
    
    LOG_INFO("Generating dependency graph for: " + target, "Graph");
    
    // TODO: Implement actual graph generation logic with BuildGraph
    std::cout << "Graph generation requires BuildGraph implementation" << std::endl;
    
    return Result<void>::Success();
}

} // namespace IgniteBT
