#include "CLI/Command.hpp"
#include "Logging/Logger.hpp"
#include "Configuration/Configuration.hpp"
#include "Platform/Platform.hpp"
#include <iostream>
#include <filesystem>

namespace IgniteBT {

class IgniteBT {
public:
    IgniteBT() {
        Initialize();
    }
    
    ~IgniteBT() {
        Shutdown();
    }
    
    int Run(int argc, char* argv[]) {
        if (argc < 2) {
            ShowUsage();
            return 1;
        }
        
        // Parse command line
        auto context = CommandParser::Parse(argc, argv);
        
        // Set engine root
        auto& config = ConfigurationManager::Get();
        config.SetEngineRoot(std::filesystem::current_path());
        
        // Load default configuration if exists
        Path configFile = config.GetEngineRoot() / "IgniteBT.json";
        if (PlatformManager::Get().FileExists(configFile)) {
            auto result = config.LoadConfiguration(configFile);
            if (result.IsFailure()) {
                LOG_WARNING(result.GetError(), "Main");
            }
        }
        
        // Execute command
        auto& registry = CommandRegistry::Get();
        auto result = registry.ExecuteCommand(context);
        
        if (result.IsFailure()) {
            LOG_ERROR(result.GetError(), "Main");
            std::cerr << "Error: " << result.GetError() << std::endl;
            return 1;
        }
        
        return 0;
    }
    
private:
    void Initialize() {
        // Initialize logging
        auto& logger = Logger::Get();
        logger.SetLogLevel(LogLevel::Info);
        
        // Register built-in commands
        RegisterCommands();
        
        LOG_INFO("IgniteBT initialized", "Main");
    }
    
    void Shutdown() {
        LOG_INFO("IgniteBT shutting down", "Main");
    }
    
    void RegisterCommands() {
        auto& registry = CommandRegistry::Get();
        
        registry.RegisterCommand(std::make_shared<HelpCommand>());
        registry.RegisterCommand(std::make_shared<VersionCommand>());
        registry.RegisterCommand(std::make_shared<CleanCommand>());
        registry.RegisterCommand(std::make_shared<RebuildCommand>());
        registry.RegisterCommand(std::make_shared<DoctorCommand>());
        registry.RegisterCommand(std::make_shared<BenchmarkCommand>());
        registry.RegisterCommand(std::make_shared<PluginsCommand>());
        registry.RegisterCommand(std::make_shared<BuildCommand>());
        registry.RegisterCommand(std::make_shared<GenerateCommand>());
        registry.RegisterCommand(std::make_shared<PackageCommand>());
        registry.RegisterCommand(std::make_shared<TestCommand>());
        registry.RegisterCommand(std::make_shared<GraphCommand>());
    }
    
    void ShowUsage() {
        std::cout << "IgniteBT - Ignite Build Tool" << std::endl;
        std::cout << "Usage: ignitebt <command> [options]" << std::endl;
        std::cout << std::endl;
        std::cout << "Use 'help' for a list of available commands." << std::endl;
    }
};

} // namespace IgniteBT

int main(int argc, char* argv[]) {
    IgniteBT::IgniteBT ignitebt;
    return ignitebt.Run(argc, argv);
}
