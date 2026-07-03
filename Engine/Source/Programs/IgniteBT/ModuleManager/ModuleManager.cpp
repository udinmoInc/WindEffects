#include "ModuleManager/ModuleManager.hpp"
#include "Platform/Platform.hpp"
#include "Logging/Logger.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <queue>

namespace IgniteBT {

// ModuleManager implementation
ModuleManager::ModuleManager() {
    // Set default search patterns
    m_includePatterns = { "src", "Source", "source", "lib", "Library" };
    m_excludePatterns = { "build", "Build", ".git", ".vs", "out", "bin" };
}

ModuleManager::~ModuleManager() {
    Clear();
}

Result<ModuleDiscoveryResult> ModuleManager::DiscoverModules(const Path& searchPath) {
    ModuleDiscoveryResult result;
    
    IPlatform* platform = PlatformManager::Get().GetPlatform();
    if (!platform) {
        return Result<ModuleDiscoveryResult>::Failure("Platform not available");
    }
    
    if (!platform->DirectoryExists(searchPath)) {
        return Result<ModuleDiscoveryResult>::Failure("Search path does not exist: " + searchPath.string());
    }
    
    Logger::Get().Info("ModuleManager: Discovering modules in " + searchPath.string());
    
    // List directories in search path
    std::vector<Path> directories = platform->ListDirectories(searchPath);
    
    for (const Path& dirPath : directories) {
        // Check if this is a module directory
        if (IsModuleDirectory(dirPath)) {
            auto moduleResult = DiscoverModuleInDirectory(dirPath);
            if (moduleResult.IsSuccess()) {
                ModuleConfig config = moduleResult.GetValue();
                result.modules.push_back(config);
                AddModule(config);
                
                if (m_discoveryCallback) {
                    m_discoveryCallback(config);
                }
                
                Logger::Get().Info("ModuleManager: Discovered module '" + config.name + "'");
            } else {
                result.errors.push_back("Failed to discover module in " + dirPath.string() + 
                                       ": " + moduleResult.GetError());
                Logger::Get().Warning("ModuleManager: {}", result.errors.back());
            }
        }
    }
    
    Logger::Get().Info("ModuleManager: Discovered " + std::to_string(result.modules.size()) + " modules");
    return Result<ModuleDiscoveryResult>::Success(result);
}

Result<ModuleDiscoveryResult> ModuleManager::DiscoverModulesRecursive(const Path& rootPath) {
    ModuleDiscoveryResult result;
    
    IPlatform* platform = PlatformManager::Get().GetPlatform();
    if (!platform) {
        return Result<ModuleDiscoveryResult>::Failure("Platform not available");
    }
    
    if (!platform->DirectoryExists(rootPath)) {
        return Result<ModuleDiscoveryResult>::Failure("Root path does not exist: " + rootPath.string());
    }
    
    Logger::Get().Info("ModuleManager: Recursively discovering modules in " + rootPath.string());
    
    // Perform recursive directory traversal
    std::vector<Path> queue = { rootPath };
    
    while (!queue.empty()) {
        Path currentPath = queue.back();
        queue.pop_back();
        
        // Check if this is a module directory
        if (IsModuleDirectory(currentPath)) {
            auto moduleResult = DiscoverModuleInDirectory(currentPath);
            if (moduleResult.IsSuccess()) {
                ModuleConfig config = moduleResult.GetValue();
                result.modules.push_back(config);
                AddModule(config);
                
                if (m_discoveryCallback) {
                    m_discoveryCallback(config);
                }
                
                Logger::Get().Info("ModuleManager: Discovered module '" + config.name + "'");
            } else {
                result.errors.push_back("Failed to discover module in " + currentPath.string() + 
                                       ": " + moduleResult.GetError());
                Logger::Get().Warning("ModuleManager: {}", result.errors.back());
            }
        } else {
            // Add subdirectories to queue for further exploration
            std::vector<Path> subdirs = platform->ListDirectories(currentPath);
            for (const Path& subdir : subdirs) {
                // Skip excluded directories
                std::string dirName = subdir.filename().string();
                bool shouldExclude = false;
                for (const std::string& pattern : m_excludePatterns) {
                    if (dirName.find(pattern) != std::string::npos) {
                        shouldExclude = true;
                        break;
                    }
                }
                
                if (!shouldExclude) {
                    queue.push_back(subdir);
                }
            }
        }
    }
    
    Logger::Get().Info("ModuleManager: Discovered " + std::to_string(result.modules.size()) + " modules recursively");
    return Result<ModuleDiscoveryResult>::Success(result);
}

bool ModuleManager::AddModule(const ModuleConfig& config) {
    if (m_modules.find(config.name) != m_modules.end()) {
        Logger::Get().Warning("ModuleManager: Module '" + config.name + "' already exists, updating");
    }
    
    m_modules[config.name] = config;
    return true;
}

bool ModuleManager::RemoveModule(const std::string& name) {
    auto it = m_modules.find(name);
    if (it == m_modules.end()) {
        return false;
    }
    
    m_modules.erase(it);
    Logger::Get().Info("ModuleManager: Removed module '" + name + "'");
    return true;
}

ModuleConfig* ModuleManager::GetModule(const std::string& name) {
    auto it = m_modules.find(name);
    return it != m_modules.end() ? &it->second : nullptr;
}

const ModuleConfig* ModuleManager::GetModule(const std::string& name) const {
    auto it = m_modules.find(name);
    return it != m_modules.end() ? &it->second : nullptr;
}

std::vector<ModuleConfig*> ModuleManager::GetModulesByType(ModuleType type) {
    std::vector<ModuleConfig*> result;
    
    for (auto& [name, config] : m_modules) {
        if (config.type == type) {
            result.push_back(&config);
        }
    }
    
    return result;
}

std::vector<ModuleConfig*> ModuleManager::GetDependencyOrderedModules() {
    std::vector<ModuleConfig*> order;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursionStack;
    bool hasCycle = false;
    
    for (auto& [name, config] : m_modules) {
        if (visited.find(name) == visited.end()) {
            VisitModuleForOrder(name, visited, recursionStack, order, hasCycle);
        }
    }
    
    if (hasCycle) {
        Logger::Get().Error("ModuleManager: Circular dependency detected in modules");
        order.clear();
    }
    
    return order;
}

void ModuleManager::VisitModuleForOrder(const std::string& moduleName,
                                       std::unordered_set<std::string>& visited,
                                       std::unordered_set<std::string>& recursionStack,
                                       std::vector<ModuleConfig*>& order,
                                       bool& hasCycle) {
    visited.insert(moduleName);
    recursionStack.insert(moduleName);
    
    ModuleConfig* module = GetModule(moduleName);
    if (module) {
        for (const std::string& depName : module->dependencies) {
            if (visited.find(depName) == visited.end()) {
                VisitModuleForOrder(depName, visited, recursionStack, order, hasCycle);
            } else if (recursionStack.find(depName) != recursionStack.end()) {
                hasCycle = true;
                Logger::Get().Error("ModuleManager: Cycle detected between modules '" + moduleName + "' and '" + depName + "'");
            }
        }
    }
    
    recursionStack.erase(moduleName);
    if (module) {
        order.push_back(module);
    }
}

Result<void> ModuleManager::ValidateDependencies() {
    for (const auto& [name, config] : m_modules) {
        for (const std::string& depName : config.dependencies) {
            if (m_modules.find(depName) == m_modules.end()) {
                return Result<void>::Failure("Module '" + name + "' depends on unknown module '" + 
                                          depName + "'");
            }
        }
    }
    
    return Result<void>::Success();
}

Result<BuildGraph> ModuleManager::CreateBuildGraph() {
    BuildGraph graph;
    
    // Add all modules to the build graph
    for (auto& [name, config] : m_modules) {
        auto result = AddModuleToBuildGraph(graph, config);
        if (!result.IsSuccess()) {
            return Result<BuildGraph>::Failure("Failed to add module '" + name + "' to build graph: " + 
                                            result.GetError());
        }
    }
    
    // Add dependencies between modules
    for (auto& [name, config] : m_modules) {
        for (const std::string& depName : config.dependencies) {
            graph.AddDependency(name, depName);
        }
    }
    
    // Validate the graph
    auto validationResult = graph.ValidateGraph();
    if (!validationResult.IsSuccess()) {
        return Result<BuildGraph>::Failure("Build graph validation failed: " + 
                                        validationResult.GetError());
    }
    
    Logger::Get().Info("ModuleManager: Created build graph with " + std::to_string(graph.GetNodeCount()) + " nodes");
    return Result<BuildGraph>::Success(std::move(graph));
}

Result<BuildGraph> ModuleManager::CreateBuildGraphForTargets(const std::vector<std::string>& targetNames) {
    // First create the full graph
    auto fullGraphResult = CreateBuildGraph();
    if (!fullGraphResult.IsSuccess()) {
        return fullGraphResult;
    }
    
    BuildGraph fullGraph = std::move(fullGraphResult.GetValue());
    
    // Create subgraph for the specified targets
    auto subgraphResult = BuildGraphUtils::CreateSubgraph(fullGraph, targetNames);
    if (!subgraphResult.IsSuccess()) {
        return Result<BuildGraph>::Failure("Failed to create subgraph: " + subgraphResult.GetError());
    }
    
    Logger::Get().Info("ModuleManager: Created build graph for " + std::to_string(targetNames.size()) + " targets");
    return Result<BuildGraph>::Success(std::move(subgraphResult.GetValue()));
}

Result<void> ModuleManager::AddModuleToBuildGraph(BuildGraph& graph, const ModuleConfig& module) {
    // Create a node for the module's target
    std::string nodeId = module.targetName.empty() ? module.name : module.targetName;
    NodeType nodeType;
    
    switch (module.type) {
        case ModuleType::Executable:
            nodeType = NodeType::Executable;
            break;
        case ModuleType::StaticLibrary:
        case ModuleType::SharedLibrary:
            nodeType = NodeType::Library;
            break;
        case ModuleType::HeaderOnly:
            nodeType = NodeType::Phony;
            break;
        default:
            nodeType = NodeType::Custom;
            break;
    }
    
    Path outputPath = module.outputDir / ModuleUtils::GetOutputFilename(module);
    BuildNode* moduleNode = graph.AddNode(nodeId, nodeType, outputPath);
    
    if (!moduleNode) {
        return Result<void>::Failure("Failed to add node for module '" + module.name + "'");
    }
    
    // Add nodes for source files
    for (const std::string& source : module.sources) {
        Path sourcePath = module.rootPath / source;
        std::string sourceId = sourcePath.string();
        
        BuildNode* sourceNode = graph.GetNode(sourceId);
        if (!sourceNode) {
            sourceNode = graph.AddNode(sourceId, NodeType::Source, sourcePath);
        }
        
        // Add dependency from module to source
        graph.AddDependency(nodeId, sourceId);
    }
    
    // Add nodes for header files
    for (const std::string& header : module.headers) {
        Path headerPath = module.rootPath / header;
        std::string headerId = headerPath.string();
        
        BuildNode* headerNode = graph.GetNode(headerId);
        if (!headerNode) {
            headerNode = graph.AddNode(headerId, NodeType::Source, headerPath);
        }
        
        // Add dependency from module to header
        graph.AddDependency(nodeId, headerId);
    }
    
    return Result<void>::Success();
}

size_t ModuleManager::GetSourceCount() const {
    size_t count = 0;
    for (const auto& [name, config] : m_modules) {
        count += config.sources.size();
    }
    return count;
}

size_t ModuleManager::GetHeaderCount() const {
    size_t count = 0;
    for (const auto& [name, config] : m_modules) {
        count += config.headers.size();
    }
    return count;
}

void ModuleManager::Clear() {
    m_modules.clear();
}

Result<ModuleConfig> ModuleManager::DiscoverModuleInDirectory(const Path& dirPath) {
    IPlatform* platform = PlatformManager::Get().GetPlatform();
    if (!platform) {
        return Result<ModuleConfig>::Failure("Platform not available");
    }
    
    ModuleConfig config;
    config.name = ModuleUtils::GenerateModuleName(dirPath);
    config.rootPath = dirPath;
    config.type = ModuleUtils::DetectModuleType(dirPath);
    
    // Set default paths
    config.sourceDir = dirPath / "src";
    config.includeDir = dirPath / "include";
    config.outputDir = dirPath / "build";
    
    // If default source dir doesn't exist, use the root directory
    if (!platform->DirectoryExists(config.sourceDir)) {
        config.sourceDir = dirPath;
    }
    
    // If default include dir doesn't exist, use the root directory
    if (!platform->DirectoryExists(config.includeDir)) {
        config.includeDir = dirPath;
    }
    
    // Find source files
    auto sourceExtensions = ModuleUtils::GetDefaultSourceExtensions();
    auto sourcePaths = FindSourceFiles(config.sourceDir, sourceExtensions);
    for (const Path& sourcePath : sourcePaths) {
        config.sources.push_back(sourcePath.string());
    }
    
    // Find header files
    auto headerExtensions = ModuleUtils::GetDefaultHeaderExtensions();
    auto headerPaths = FindHeaderFiles(config.includeDir, headerExtensions);
    for (const Path& headerPath : headerPaths) {
        config.headers.push_back(headerPath.string());
    }
    
    // Set target name
    config.targetName = ModuleUtils::GetTargetName(config);
    
    Logger::Get().Debug("ModuleManager: Discovered module '" + config.name + "' with " + std::to_string(config.sources.size()) + " sources and " + std::to_string(config.headers.size()) + " headers");
    
    return Result<ModuleConfig>::Success(config);
}

bool ModuleManager::IsModuleDirectory(const Path& dirPath) {
    IPlatform* platform = PlatformManager::Get().GetPlatform();
    if (!platform) {
        return false;
    }
    
    // Check if directory contains any source files
    auto sourceExtensions = ModuleUtils::GetDefaultSourceExtensions();
    std::vector<Path> sourceFiles = FindSourceFiles(dirPath, sourceExtensions);
    
    if (!sourceFiles.empty()) {
        return true;
    }
    
    // Check for common module indicators
    std::vector<Path> subdirs = platform->ListDirectories(dirPath);
    for (const Path& subdir : subdirs) {
        std::string dirName = subdir.filename().string();
        if (dirName == "src" || dirName == "include" || dirName == "Source" || dirName == "Include") {
            return true;
        }
    }
    
    return false;
}

std::vector<Path> ModuleManager::FindSourceFiles(const Path& dir, const std::vector<std::string>& extensions) {
    std::vector<Path> sourceFiles;
    
    IPlatform* platform = PlatformManager::Get().GetPlatform();
    if (!platform || !platform->DirectoryExists(dir)) {
        return sourceFiles;
    }
    
    for (const std::string& ext : extensions) {
        std::string pattern = "*." + ext;
        std::vector<Path> files = platform->ListFiles(dir, pattern);
        sourceFiles.insert(sourceFiles.end(), files.begin(), files.end());
    }
    
    return sourceFiles;
}

std::vector<Path> ModuleManager::FindHeaderFiles(const Path& dir, const std::vector<std::string>& extensions) {
    std::vector<Path> headerFiles;
    
    IPlatform* platform = PlatformManager::Get().GetPlatform();
    if (!platform || !platform->DirectoryExists(dir)) {
        return headerFiles;
    }
    
    for (const std::string& ext : extensions) {
        std::string pattern = "*." + ext;
        std::vector<Path> files = platform->ListFiles(dir, pattern);
        headerFiles.insert(headerFiles.end(), files.begin(), files.end());
    }
    
    return headerFiles;
}

// ModuleUtils implementation
namespace ModuleUtils {

std::vector<std::string> GetDefaultSourceExtensions() {
    return { "cpp", "cc", "cxx", "c", "m", "mm" };
}

std::vector<std::string> GetDefaultHeaderExtensions() {
    return { "h", "hpp", "hxx", "inl", "inc" };
}

ModuleType DetectModuleType(const Path& dirPath) {
    IPlatform* platform = PlatformManager::Get().GetPlatform();
    if (!platform) {
        return ModuleType::StaticLibrary;
    }
    
    std::string dirName = dirPath.filename().string();
    
    // Check for common naming patterns
    if (dirName.find("app") != std::string::npos || 
        dirName.find("exe") != std::string::npos ||
        dirName.find("main") != std::string::npos) {
        return ModuleType::Executable;
    }
    
    if (dirName.find("lib") != std::string::npos) {
        return ModuleType::StaticLibrary;
    }
    
    if (dirName.find("dll") != std::string::npos || 
        dirName.find("so") != std::string::npos) {
        return ModuleType::SharedLibrary;
    }
    
    if (dirName.find("plugin") != std::string::npos) {
        return ModuleType::Plugin;
    }
    
    // Default to static library
    return ModuleType::StaticLibrary;
}

std::string GenerateModuleName(const Path& dirPath) {
    std::string dirName = dirPath.filename().string();
    
    // Convert to lowercase and replace spaces with underscores
    std::string moduleName;
    for (char c : dirName) {
        if (c == ' ' || c == '-') {
            moduleName += '_';
        } else {
            moduleName += std::tolower(c);
        }
    }
    
    return moduleName;
}

Result<void> ValidateModuleConfig(const ModuleConfig& config) {
    if (config.name.empty()) {
        return Result<void>::Failure("Module name cannot be empty");
    }
    
    if (config.rootPath.empty()) {
        return Result<void>::Failure("Module root path cannot be empty");
    }
    
    if (config.sources.empty() && config.type != ModuleType::HeaderOnly && 
        config.type != ModuleType::Interface) {
        return Result<void>::Failure("Module must have source files");
    }
    
    return Result<void>::Success();
}

void ResolvePaths(ModuleConfig& config, const Path& basePath) {
    if (!config.rootPath.is_absolute()) {
        config.rootPath = basePath / config.rootPath;
    }
    
    if (!config.sourceDir.is_absolute()) {
        config.sourceDir = config.rootPath / config.sourceDir;
    }
    
    if (!config.includeDir.is_absolute()) {
        config.includeDir = config.rootPath / config.includeDir;
    }
    
    if (!config.outputDir.is_absolute()) {
        config.outputDir = config.rootPath / config.outputDir;
    }
}

std::string GetOutputFilename(const ModuleConfig& config) {
    std::string filename = config.targetName.empty() ? config.name : config.targetName;
    
    switch (config.type) {
        case ModuleType::Executable:
            #ifdef _WIN32
            filename += ".exe";
            #endif
            break;
        case ModuleType::StaticLibrary:
            #ifdef _WIN32
            filename = filename + ".lib";
            #else
            filename = "lib" + filename + ".a";
            #endif
            break;
        case ModuleType::SharedLibrary:
            #ifdef _WIN32
            filename += ".dll";
            #elif __APPLE__
            filename = "lib" + filename + ".dylib";
            #else
            filename = "lib" + filename + ".so";
            #endif
            break;
        default:
            break;
    }
    
    return filename;
}

std::string GetTargetName(const ModuleConfig& config) {
    if (!config.targetName.empty()) {
        return config.targetName;
    }
    
    return config.name;
}

ModuleConfig MergeModuleConfigs(const ModuleConfig& base, const ModuleConfig& override) {
    ModuleConfig merged = base;
    
    if (!override.targetName.empty()) {
        merged.targetName = override.targetName;
    }
    
    if (!override.outputDir.empty()) {
        merged.outputDir = override.outputDir;
    }
    
    if (!override.sourceDir.empty()) {
        merged.sourceDir = override.sourceDir;
    }
    
    if (!override.includeDir.empty()) {
        merged.includeDir = override.includeDir;
    }
    
    // Merge source and header lists
    for (const std::string& source : override.sources) {
        if (std::find(merged.sources.begin(), merged.sources.end(), source) == merged.sources.end()) {
            merged.sources.push_back(source);
        }
    }
    
    for (const std::string& header : override.headers) {
        if (std::find(merged.headers.begin(), merged.headers.end(), header) == merged.headers.end()) {
            merged.headers.push_back(header);
        }
    }
    
    // Merge dependencies
    for (const std::string& dep : override.dependencies) {
        if (std::find(merged.dependencies.begin(), merged.dependencies.end(), dep) == merged.dependencies.end()) {
            merged.dependencies.push_back(dep);
        }
    }
    
    // Merge compile and link flags
    for (const std::string& flag : override.compileFlags) {
        if (std::find(merged.compileFlags.begin(), merged.compileFlags.end(), flag) == merged.compileFlags.end()) {
            merged.compileFlags.push_back(flag);
        }
    }
    
    for (const std::string& flag : override.linkFlags) {
        if (std::find(merged.linkFlags.begin(), merged.linkFlags.end(), flag) == merged.linkFlags.end()) {
            merged.linkFlags.push_back(flag);
        }
    }
    
    // Merge defines
    for (const std::string& define : override.defines) {
        if (std::find(merged.defines.begin(), merged.defines.end(), define) == merged.defines.end()) {
            merged.defines.push_back(define);
        }
    }
    
    return merged;
}

bool IsDependency(const ModuleConfig& module, const ModuleConfig& potentialDep) {
    for (const std::string& depName : module.dependencies) {
        if (depName == potentialDep.name) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> GetTransitiveDependencies(const ModuleManager& manager, 
                                                   const std::string& moduleName) {
    std::vector<std::string> dependencies;
    std::unordered_set<std::string> visited;
    std::queue<std::string> queue;
    
    const ModuleConfig* module = manager.GetModule(moduleName);
    if (!module) {
        return dependencies;
    }
    
    for (const std::string& dep : module->dependencies) {
        queue.push(dep);
    }
    
    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop();
        
        if (visited.find(current) != visited.end()) {
            continue;
        }
        
        visited.insert(current);
        dependencies.push_back(current);
        
        const ModuleConfig* currentModule = manager.GetModule(current);
        if (currentModule) {
            for (const std::string& dep : currentModule->dependencies) {
                queue.push(dep);
            }
        }
    }
    
    return dependencies;
}

} // namespace ModuleUtils

} // namespace IgniteBT
