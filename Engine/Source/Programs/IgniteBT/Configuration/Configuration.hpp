#pragma once

#include "Core/Types.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

namespace IgniteBT {

using json = nlohmann::json;

// Build configuration
struct BuildConfig {
    BuildConfiguration configuration = BuildConfiguration::Development;
    Platform platform = Platform::Windows;
    Architecture architecture = Architecture::x64;
    CompilerType compiler = CompilerType::MSVC;
    
    bool enableUnityBuild = false;
    bool enableCache = true;
    bool enableParallelBuild = true;
    int parallelJobs = 0; // 0 = auto-detect
    
    bool enablePCH = true;
    bool enableFastLink = true;
    
    std::string outputDirectory = "Build";
    std::string intermediateDirectory = "Build/Intermediate";
    
    std::vector<std::string> defines;
    std::vector<std::string> includePaths;
    std::vector<std::string> libraryPaths;
    std::vector<std::string> libraries;
    
    std::string compilerFlags;
    std::string linkerFlags;
    
    int optimizationLevel = 2;
    bool enableDebugSymbols = true;
    bool enableSanitizers = false;
    
    static BuildConfig FromJson(const json& j);
    json ToJson() const;
};

// Target configuration
struct TargetConfig {
    std::string name;
    TargetType type = TargetType::Executable;
    std::string sourceDirectory;
    std::vector<std::string> sourceFiles;
    std::vector<std::string> publicHeaders;
    std::vector<std::string> privateHeaders;
    
    std::vector<std::string> dependencies;
    std::vector<std::string> publicDependencies;
    
    std::vector<std::string> defines;
    std::vector<std::string> includePaths;
    std::vector<std::string> libraryPaths;
    std::vector<std::string> libraries;
    
    std::string precompiledHeader;
    bool enableUnityBuild = false;
    
    static TargetConfig FromJson(const json& j);
    json ToJson() const;
};

// Module configuration
struct ModuleConfig {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    
    std::vector<std::string> dependencies;
    std::vector<std::string> targets;
    
    std::vector<std::string> platforms;
    std::vector<std::string> configurations;
    
    static ModuleConfig FromJson(const json& j);
    json ToJson() const;
};

// Plugin configuration
struct PluginConfig {
    std::string name;
    std::string version;
    std::string description;
    
    std::vector<std::string> dependencies;
    std::vector<std::string> modules;
    
    std::vector<std::string> platforms;
    
    static PluginConfig FromJson(const json& j);
    json ToJson() const;
};

// Configuration manager
class ConfigurationManager {
public:
    static ConfigurationManager& Get();
    
    Result<void> LoadConfiguration(const Path& configFile);
    Result<void> SaveConfiguration(const Path& configFile);
    
    const BuildConfig& GetBuildConfig() const;
    BuildConfig& GetBuildConfig();
    
    Result<void> AddTargetConfig(const TargetConfig& config);
    Result<TargetConfig> GetTargetConfig(const std::string& name) const;
    std::vector<TargetConfig> GetAllTargetConfigs() const;
    
    Result<void> AddModuleConfig(const ModuleConfig& config);
    Result<ModuleConfig> GetModuleConfig(const std::string& name) const;
    std::vector<ModuleConfig> GetAllModuleConfigs() const;
    
    Result<void> AddPluginConfig(const PluginConfig& config);
    Result<PluginConfig> GetPluginConfig(const std::string& name) const;
    std::vector<PluginConfig> GetAllPluginConfigs() const;
    
    void SetEngineRoot(const Path& root);
    Path GetEngineRoot() const;
    
private:
    ConfigurationManager() = default;
    
    BuildConfig m_buildConfig;
    std::unordered_map<std::string, TargetConfig> m_targetConfigs;
    std::unordered_map<std::string, ModuleConfig> m_moduleConfigs;
    std::unordered_map<std::string, PluginConfig> m_pluginConfigs;
    
    Path m_engineRoot;
    Path m_configFile;
};

} // namespace IgniteBT
