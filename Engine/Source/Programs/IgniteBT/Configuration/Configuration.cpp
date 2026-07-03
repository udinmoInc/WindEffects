#include "Configuration/Configuration.hpp"
#include "Logging/Logger.hpp"
#include <fstream>

namespace IgniteBT {

// BuildConfig
BuildConfig BuildConfig::FromJson(const json& j) {
    BuildConfig config;
    
    if (j.contains("configuration")) {
        std::string configStr = j["configuration"];
        if (configStr == "Debug") config.configuration = BuildConfiguration::Debug;
        else if (configStr == "Development") config.configuration = BuildConfiguration::Development;
        else if (configStr == "Profile") config.configuration = BuildConfiguration::Profile;
        else if (configStr == "Shipping") config.configuration = BuildConfiguration::Shipping;
    }
    
    if (j.contains("platform")) {
        std::string platformStr = j["platform"];
        if (platformStr == "Windows") config.platform = Platform::Windows;
        else if (platformStr == "Linux") config.platform = Platform::Linux;
        else if (platformStr == "Mac") config.platform = Platform::Mac;
    }
    
    if (j.contains("architecture")) {
        std::string archStr = j["architecture"];
        if (archStr == "x64") config.architecture = Architecture::x64;
        else if (archStr == "ARM64") config.architecture = Architecture::ARM64;
    }
    
    if (j.contains("compiler")) {
        std::string compilerStr = j["compiler"];
        if (compilerStr == "MSVC") config.compiler = CompilerType::MSVC;
        else if (compilerStr == "GCC") config.compiler = CompilerType::GCC;
        else if (compilerStr == "Clang") config.compiler = CompilerType::Clang;
        else if (compilerStr == "AppleClang") config.compiler = CompilerType::AppleClang;
    }
    
    if (j.contains("enableUnityBuild")) config.enableUnityBuild = j["enableUnityBuild"];
    if (j.contains("enableCache")) config.enableCache = j["enableCache"];
    if (j.contains("enableParallelBuild")) config.enableParallelBuild = j["enableParallelBuild"];
    if (j.contains("parallelJobs")) config.parallelJobs = j["parallelJobs"];
    if (j.contains("enablePCH")) config.enablePCH = j["enablePCH"];
    if (j.contains("enableFastLink")) config.enableFastLink = j["enableFastLink"];
    
    if (j.contains("outputDirectory")) config.outputDirectory = j["outputDirectory"];
    if (j.contains("intermediateDirectory")) config.intermediateDirectory = j["intermediateDirectory"];
    
    if (j.contains("defines")) config.defines = j["defines"].get<std::vector<std::string>>();
    if (j.contains("includePaths")) config.includePaths = j["includePaths"].get<std::vector<std::string>>();
    if (j.contains("libraryPaths")) config.libraryPaths = j["libraryPaths"].get<std::vector<std::string>>();
    if (j.contains("libraries")) config.libraries = j["libraries"].get<std::vector<std::string>>();
    
    if (j.contains("compilerFlags")) config.compilerFlags = j["compilerFlags"];
    if (j.contains("linkerFlags")) config.linkerFlags = j["linkerFlags"];
    
    if (j.contains("optimizationLevel")) config.optimizationLevel = j["optimizationLevel"];
    if (j.contains("enableDebugSymbols")) config.enableDebugSymbols = j["enableDebugSymbols"];
    if (j.contains("enableSanitizers")) config.enableSanitizers = j["enableSanitizers"];
    
    return config;
}

json BuildConfig::ToJson() const {
    json j;
    
    std::string configStr;
    switch (configuration) {
        case BuildConfiguration::Debug: configStr = "Debug"; break;
        case BuildConfiguration::Development: configStr = "Development"; break;
        case BuildConfiguration::Profile: configStr = "Profile"; break;
        case BuildConfiguration::Shipping: configStr = "Shipping"; break;
        case BuildConfiguration::Custom: configStr = "Custom"; break;
    }
    j["configuration"] = configStr;
    
    std::string platformStr;
    switch (platform) {
        case Platform::Windows: platformStr = "Windows"; break;
        case Platform::Linux: platformStr = "Linux"; break;
        case Platform::Mac: platformStr = "Mac"; break;
        default: platformStr = "Unknown"; break;
    }
    j["platform"] = platformStr;
    
    std::string archStr;
    switch (architecture) {
        case Architecture::x64: archStr = "x64"; break;
        case Architecture::ARM64: archStr = "ARM64"; break;
        default: archStr = "Unknown"; break;
    }
    j["architecture"] = archStr;
    
    std::string compilerStr;
    switch (compiler) {
        case CompilerType::MSVC: compilerStr = "MSVC"; break;
        case CompilerType::GCC: compilerStr = "GCC"; break;
        case CompilerType::Clang: compilerStr = "Clang"; break;
        case CompilerType::AppleClang: compilerStr = "AppleClang"; break;
        default: compilerStr = "Unknown"; break;
    }
    j["compiler"] = compilerStr;
    
    j["enableUnityBuild"] = enableUnityBuild;
    j["enableCache"] = enableCache;
    j["enableParallelBuild"] = enableParallelBuild;
    j["parallelJobs"] = parallelJobs;
    j["enablePCH"] = enablePCH;
    j["enableFastLink"] = enableFastLink;
    
    j["outputDirectory"] = outputDirectory;
    j["intermediateDirectory"] = intermediateDirectory;
    
    j["defines"] = defines;
    j["includePaths"] = includePaths;
    j["libraryPaths"] = libraryPaths;
    j["libraries"] = libraries;
    
    j["compilerFlags"] = compilerFlags;
    j["linkerFlags"] = linkerFlags;
    
    j["optimizationLevel"] = optimizationLevel;
    j["enableDebugSymbols"] = enableDebugSymbols;
    j["enableSanitizers"] = enableSanitizers;
    
    return j;
}

// TargetConfig
TargetConfig TargetConfig::FromJson(const json& j) {
    TargetConfig config;
    
    if (j.contains("name")) config.name = j["name"];
    if (j.contains("type")) {
        std::string typeStr = j["type"];
        if (typeStr == "Executable") config.type = TargetType::Executable;
        else if (typeStr == "SharedLibrary") config.type = TargetType::SharedLibrary;
        else if (typeStr == "StaticLibrary") config.type = TargetType::StaticLibrary;
        else if (typeStr == "Interface") config.type = TargetType::Interface;
        else if (typeStr == "Module") config.type = TargetType::Module;
    }
    if (j.contains("sourceDirectory")) config.sourceDirectory = j["sourceDirectory"];
    if (j.contains("sourceFiles")) config.sourceFiles = j["sourceFiles"].get<std::vector<std::string>>();
    if (j.contains("publicHeaders")) config.publicHeaders = j["publicHeaders"].get<std::vector<std::string>>();
    if (j.contains("privateHeaders")) config.privateHeaders = j["privateHeaders"].get<std::vector<std::string>>();
    
    if (j.contains("dependencies")) config.dependencies = j["dependencies"].get<std::vector<std::string>>();
    if (j.contains("publicDependencies")) config.publicDependencies = j["publicDependencies"].get<std::vector<std::string>>();
    
    if (j.contains("defines")) config.defines = j["defines"].get<std::vector<std::string>>();
    if (j.contains("includePaths")) config.includePaths = j["includePaths"].get<std::vector<std::string>>();
    if (j.contains("libraryPaths")) config.libraryPaths = j["libraryPaths"].get<std::vector<std::string>>();
    if (j.contains("libraries")) config.libraries = j["libraries"].get<std::vector<std::string>>();
    
    if (j.contains("precompiledHeader")) config.precompiledHeader = j["precompiledHeader"];
    if (j.contains("enableUnityBuild")) config.enableUnityBuild = j["enableUnityBuild"];
    
    return config;
}

json TargetConfig::ToJson() const {
    json j;
    
    j["name"] = name;
    
    std::string typeStr;
    switch (type) {
        case TargetType::Executable: typeStr = "Executable"; break;
        case TargetType::SharedLibrary: typeStr = "SharedLibrary"; break;
        case TargetType::StaticLibrary: typeStr = "StaticLibrary"; break;
        case TargetType::Interface: typeStr = "Interface"; break;
        case TargetType::Module: typeStr = "Module"; break;
    }
    j["type"] = typeStr;
    
    j["sourceDirectory"] = sourceDirectory;
    j["sourceFiles"] = sourceFiles;
    j["publicHeaders"] = publicHeaders;
    j["privateHeaders"] = privateHeaders;
    
    j["dependencies"] = dependencies;
    j["publicDependencies"] = publicDependencies;
    
    j["defines"] = defines;
    j["includePaths"] = includePaths;
    j["libraryPaths"] = libraryPaths;
    j["libraries"] = libraries;
    
    j["precompiledHeader"] = precompiledHeader;
    j["enableUnityBuild"] = enableUnityBuild;
    
    return j;
}

// ModuleConfig
ModuleConfig ModuleConfig::FromJson(const json& j) {
    ModuleConfig config;
    
    if (j.contains("name")) config.name = j["name"];
    if (j.contains("version")) config.version = j["version"];
    if (j.contains("description")) config.description = j["description"];
    if (j.contains("author")) config.author = j["author"];
    
    if (j.contains("dependencies")) config.dependencies = j["dependencies"].get<std::vector<std::string>>();
    if (j.contains("targets")) config.targets = j["targets"].get<std::vector<std::string>>();
    
    if (j.contains("platforms")) config.platforms = j["platforms"].get<std::vector<std::string>>();
    if (j.contains("configurations")) config.configurations = j["configurations"].get<std::vector<std::string>>();
    
    return config;
}

json ModuleConfig::ToJson() const {
    json j;
    
    j["name"] = name;
    j["version"] = version;
    j["description"] = description;
    j["author"] = author;
    
    j["dependencies"] = dependencies;
    j["targets"] = targets;
    
    j["platforms"] = platforms;
    j["configurations"] = configurations;
    
    return j;
}

// PluginConfig
PluginConfig PluginConfig::FromJson(const json& j) {
    PluginConfig config;
    
    if (j.contains("name")) config.name = j["name"];
    if (j.contains("version")) config.version = j["version"];
    if (j.contains("description")) config.description = j["description"];
    
    if (j.contains("dependencies")) config.dependencies = j["dependencies"].get<std::vector<std::string>>();
    if (j.contains("modules")) config.modules = j["modules"].get<std::vector<std::string>>();
    
    if (j.contains("platforms")) config.platforms = j["platforms"].get<std::vector<std::string>>();
    
    return config;
}

json PluginConfig::ToJson() const {
    json j;
    
    j["name"] = name;
    j["version"] = version;
    j["description"] = description;
    
    j["dependencies"] = dependencies;
    j["modules"] = modules;
    
    j["platforms"] = platforms;
    
    return j;
}

// ConfigurationManager
ConfigurationManager& ConfigurationManager::Get() {
    static ConfigurationManager instance;
    return instance;
}

Result<void> ConfigurationManager::LoadConfiguration(const Path& configFile) {
    m_configFile = configFile;
    
    std::ifstream file(configFile);
    if (!file.is_open()) {
        return Result<void>::Failure("Failed to open configuration file: " + configFile.string());
    }
    
    try {
        json j;
        file >> j;
        
        if (j.contains("build")) {
            m_buildConfig = BuildConfig::FromJson(j["build"]);
        }
        
        if (j.contains("targets")) {
            for (const auto& targetJson : j["targets"]) {
                TargetConfig config = TargetConfig::FromJson(targetJson);
                m_targetConfigs[config.name] = config;
            }
        }
        
        if (j.contains("modules")) {
            for (const auto& moduleJson : j["modules"]) {
                ModuleConfig config = ModuleConfig::FromJson(moduleJson);
                m_moduleConfigs[config.name] = config;
            }
        }
        
        if (j.contains("plugins")) {
            for (const auto& pluginJson : j["plugins"]) {
                PluginConfig config = PluginConfig::FromJson(pluginJson);
                m_pluginConfigs[config.name] = config;
            }
        }
        
        LOG_INFO("Configuration loaded successfully", "Config");
        return Result<void>::Success();
        
    } catch (const json::exception& e) {
        return Result<void>::Failure("Failed to parse JSON: " + std::string(e.what()));
    }
}

Result<void> ConfigurationManager::SaveConfiguration(const Path& configFile) {
    json j;
    
    j["build"] = m_buildConfig.ToJson();
    
    json targetsJson = json::array();
    for (const auto& [name, config] : m_targetConfigs) {
        targetsJson.push_back(config.ToJson());
    }
    j["targets"] = targetsJson;
    
    json modulesJson = json::array();
    for (const auto& [name, config] : m_moduleConfigs) {
        modulesJson.push_back(config.ToJson());
    }
    j["modules"] = modulesJson;
    
    json pluginsJson = json::array();
    for (const auto& [name, config] : m_pluginConfigs) {
        pluginsJson.push_back(config.ToJson());
    }
    j["plugins"] = pluginsJson;
    
    std::ofstream file(configFile);
    if (!file.is_open()) {
        return Result<void>::Failure("Failed to open configuration file for writing: " + configFile.string());
    }
    
    file << j.dump(4);
    
    LOG_INFO("Configuration saved successfully", "Config");
    return Result<void>::Success();
}

const BuildConfig& ConfigurationManager::GetBuildConfig() const {
    return m_buildConfig;
}

BuildConfig& ConfigurationManager::GetBuildConfig() {
    return m_buildConfig;
}

Result<void> ConfigurationManager::AddTargetConfig(const TargetConfig& config) {
    if (m_targetConfigs.find(config.name) != m_targetConfigs.end()) {
        return Result<void>::Failure("Target config already exists: " + config.name);
    }
    m_targetConfigs[config.name] = config;
    return Result<void>::Success();
}

Result<TargetConfig> ConfigurationManager::GetTargetConfig(const std::string& name) const {
    auto it = m_targetConfigs.find(name);
    if (it == m_targetConfigs.end()) {
        return Result<TargetConfig>::Failure("Target config not found: " + name);
    }
    return Result<TargetConfig>::Success(it->second);
}

std::vector<TargetConfig> ConfigurationManager::GetAllTargetConfigs() const {
    std::vector<TargetConfig> configs;
    for (const auto& [name, config] : m_targetConfigs) {
        configs.push_back(config);
    }
    return configs;
}

Result<void> ConfigurationManager::AddModuleConfig(const ModuleConfig& config) {
    if (m_moduleConfigs.find(config.name) != m_moduleConfigs.end()) {
        return Result<void>::Failure("Module config already exists: " + config.name);
    }
    m_moduleConfigs[config.name] = config;
    return Result<void>::Success();
}

Result<ModuleConfig> ConfigurationManager::GetModuleConfig(const std::string& name) const {
    auto it = m_moduleConfigs.find(name);
    if (it == m_moduleConfigs.end()) {
        return Result<ModuleConfig>::Failure("Module config not found: " + name);
    }
    return Result<ModuleConfig>::Success(it->second);
}

std::vector<ModuleConfig> ConfigurationManager::GetAllModuleConfigs() const {
    std::vector<ModuleConfig> configs;
    for (const auto& [name, config] : m_moduleConfigs) {
        configs.push_back(config);
    }
    return configs;
}

Result<void> ConfigurationManager::AddPluginConfig(const PluginConfig& config) {
    if (m_pluginConfigs.find(config.name) != m_pluginConfigs.end()) {
        return Result<void>::Failure("Plugin config already exists: " + config.name);
    }
    m_pluginConfigs[config.name] = config;
    return Result<void>::Success();
}

Result<PluginConfig> ConfigurationManager::GetPluginConfig(const std::string& name) const {
    auto it = m_pluginConfigs.find(name);
    if (it == m_pluginConfigs.end()) {
        return Result<PluginConfig>::Failure("Plugin config not found: " + name);
    }
    return Result<PluginConfig>::Success(it->second);
}

std::vector<PluginConfig> ConfigurationManager::GetAllPluginConfigs() const {
    std::vector<PluginConfig> configs;
    for (const auto& [name, config] : m_pluginConfigs) {
        configs.push_back(config);
    }
    return configs;
}

void ConfigurationManager::SetEngineRoot(const Path& root) {
    m_engineRoot = root;
}

Path ConfigurationManager::GetEngineRoot() const {
    return m_engineRoot;
}

} // namespace IgniteBT
