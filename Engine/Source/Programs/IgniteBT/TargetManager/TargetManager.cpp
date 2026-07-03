#include "TargetManager/TargetManager.hpp"
#include "Platform/Platform.hpp"
#include "Logging/Logger.hpp"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <queue>

namespace IgniteBT {

// TargetManager implementation
TargetManager::TargetManager()
    : m_defaultConfig(BuildConfig::Release) {
}

TargetManager::~TargetManager() {
    Clear();
}

bool TargetManager::AddTarget(const TargetConfig& config) {
    auto validationResult = ValidateTargetConfig(config);
    if (!validationResult.IsSuccess()) {
        Logger::Get().Error("TargetManager: Failed to add target '" + config.name + "': " + validationResult.GetError());
        return false;
    }
    
    if (m_targets.find(config.name) != m_targets.end()) {
        Logger::Get().Warning("TargetManager: Target '" + config.name + "' already exists, updating");
    }
    
    m_targets[config.name] = config;
    Logger::Get().Info("TargetManager: Added target '" + config.name + "'");
    return true;
}

bool TargetManager::RemoveTarget(const std::string& name) {
    auto it = m_targets.find(name);
    if (it == m_targets.end()) {
        return false;
    }
    
    m_targets.erase(it);
    Logger::Get().Info("TargetManager: Removed target '" + name + "'");
    return true;
}

TargetConfig* TargetManager::GetTarget(const std::string& name) {
    auto it = m_targets.find(name);
    return it != m_targets.end() ? &it->second : nullptr;
}

const TargetConfig* TargetManager::GetTarget(const std::string& name) const {
    auto it = m_targets.find(name);
    return it != m_targets.end() ? &it->second : nullptr;
}

std::vector<TargetConfig*> TargetManager::GetTargetsByType(TargetType type) {
    std::vector<TargetConfig*> result;
    
    for (auto& [name, config] : m_targets) {
        if (config.type == type) {
            result.push_back(&config);
        }
    }
    
    return result;
}

std::vector<TargetConfig*> TargetManager::GetTargetsByConfig(BuildConfig config) {
    std::vector<TargetConfig*> result;
    
    for (auto& [name, targetConfig] : m_targets) {
        if (targetConfig.config == config) {
            result.push_back(&targetConfig);
        }
    }
    
    return result;
}

std::vector<TargetConfig*> TargetManager::GetDependencyOrderedTargets() {
    std::vector<TargetConfig*> order;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursionStack;
    bool hasCycle = false;
    
    for (auto& [name, config] : m_targets) {
        if (visited.find(name) == visited.end()) {
            VisitTargetForOrder(name, visited, recursionStack, order, hasCycle);
        }
    }
    
    if (hasCycle) {
        Logger::Get().Error("TargetManager: Circular dependency detected in targets");
        order.clear();
    }
    
    return order;
}

void TargetManager::VisitTargetForOrder(const std::string& targetName,
                                       std::unordered_set<std::string>& visited,
                                       std::unordered_set<std::string>& recursionStack,
                                       std::vector<TargetConfig*>& order,
                                       bool& hasCycle) {
    visited.insert(targetName);
    recursionStack.insert(targetName);
    
    TargetConfig* target = GetTarget(targetName);
    if (target) {
        for (const std::string& depName : target->dependencies) {
            if (visited.find(depName) == visited.end()) {
                VisitTargetForOrder(depName, visited, recursionStack, order, hasCycle);
            } else if (recursionStack.find(depName) != recursionStack.end()) {
                hasCycle = true;
                Logger::Get().Error("TargetManager: Cycle detected between targets '" + targetName + "' and '" + depName + "'");
            }
        }
    }
    
    recursionStack.erase(targetName);
    if (target) {
        order.push_back(target);
    }
}

Result<void> TargetManager::ValidateDependencies() {
    for (const auto& [name, config] : m_targets) {
        for (const std::string& depName : config.dependencies) {
            if (m_targets.find(depName) == m_targets.end()) {
                return Result<void>::Failure("Target '" + name + "' depends on unknown target '" + 
                                          depName + "'");
            }
        }
    }
    
    return Result<void>::Success();
}

Result<BuildResult> TargetManager::BuildTarget(const std::string& targetName) {
    TargetConfig* target = GetTarget(targetName);
    if (!target) {
        return Result<BuildResult>::Failure("Target '" + targetName + "' not found");
    }
    
    Logger::Get().Info("TargetManager: Building target '" + targetName + "'");
    
    auto startTime = std::chrono::steady_clock::now();
    
    auto result = ExecuteBuild(*target);
    
    auto endTime = std::chrono::steady_clock::now();
    double buildTime = std::chrono::duration<double>(endTime - startTime).count();
    
    if (result.IsSuccess()) {
        BuildResult buildResult = result.GetValue();
        buildResult.buildTime = buildTime;
        buildResult.startTime = startTime;
        buildResult.endTime = endTime;
        
        m_buildHistory.push_back(buildResult);
        
        if (m_buildCompleteCallback) {
            m_buildCompleteCallback(buildResult);
        }
        
        Logger::Get().Info("TargetManager: Built target '" + targetName + "' in " + std::to_string(buildTime) + " seconds");
        return Result<BuildResult>::Success(buildResult);
    } else {
        BuildResult buildResult;
        buildResult.targetName = targetName;
        buildResult.success = false;
        buildResult.errors = result.GetError();
        buildResult.buildTime = buildTime;
        buildResult.startTime = startTime;
        buildResult.endTime = endTime;
        
        m_buildHistory.push_back(buildResult);
        
        if (m_buildCompleteCallback) {
            m_buildCompleteCallback(buildResult);
        }
        
        Logger::Get().Error("TargetManager: Failed to build target '" + targetName + "': " + result.GetError());
        return Result<BuildResult>::Failure(result.GetError());
    }
}

Result<std::vector<BuildResult>> TargetManager::BuildTargets(const std::vector<std::string>& targetNames) {
    std::vector<BuildResult> results;
    
    // Get dependency-ordered targets
    auto orderedTargets = GetDependencyOrderedTargets();
    
    // Filter to only requested targets and their dependencies
    std::unordered_set<std::string> targetSet(targetNames.begin(), targetNames.end());
    std::unordered_set<std::string> targetsToBuild;
    
    for (TargetConfig* target : orderedTargets) {
        if (targetSet.find(target->name) != targetSet.end()) {
            // Add this target and all its dependencies
            targetsToBuild.insert(target->name);
            for (const std::string& dep : target->dependencies) {
                targetsToBuild.insert(dep);
            }
        }
    }
    
    // Build targets in order
    for (TargetConfig* target : orderedTargets) {
        if (targetsToBuild.find(target->name) != targetsToBuild.end()) {
            auto result = BuildTarget(target->name);
            if (result.IsSuccess()) {
                results.push_back(result.GetValue());
            } else {
                return Result<std::vector<BuildResult>>::Failure("Failed to build target '" + target->name + "'");
            }
        }
    }
    
    return Result<std::vector<BuildResult>>::Success(results);
}

Result<std::vector<BuildResult>> TargetManager::BuildAll() {
    std::vector<std::string> targetNames;
    for (const auto& [name, config] : m_targets) {
        targetNames.push_back(name);
    }
    
    return BuildTargets(targetNames);
}

bool TargetManager::CleanTarget(const std::string& targetName) {
    TargetConfig* target = GetTarget(targetName);
    if (!target) {
        return false;
    }
    
    IPlatform* platform = PlatformManager::Get().GetPlatform();
    if (!platform) {
        return false;
    }
    
    Path outputPath = GetTargetOutputPath(*target);
    if (platform->FileExists(outputPath)) {
        platform->DeleteFile(outputPath);
        Logger::Get().Info("TargetManager: Cleaned target '" + targetName + "'");
    }
    
    // Clean intermediate files
    Path intermediatePath = GetTargetIntermediatePath(*target);
    if (platform->DirectoryExists(intermediatePath)) {
        platform->DeleteDirectory(intermediatePath, true);
    }
    
    return true;
}

bool TargetManager::CleanTargets(const std::vector<std::string>& targetNames) {
    bool allSuccess = true;
    for (const std::string& targetName : targetNames) {
        if (!CleanTarget(targetName)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool TargetManager::CleanAll() {
    std::vector<std::string> targetNames;
    for (const auto& [name, config] : m_targets) {
        targetNames.push_back(name);
    }
    return CleanTargets(targetNames);
}

Result<BuildResult> TargetManager::RebuildTarget(const std::string& targetName) {
    CleanTarget(targetName);
    return BuildTarget(targetName);
}

Result<std::vector<BuildResult>> TargetManager::RebuildTargets(const std::vector<std::string>& targetNames) {
    CleanTargets(targetNames);
    return BuildTargets(targetNames);
}

Result<std::vector<BuildResult>> TargetManager::RebuildAll() {
    CleanAll();
    return BuildAll();
}

size_t TargetManager::GetSourceCount() const {
    size_t count = 0;
    for (const auto& [name, config] : m_targets) {
        count += config.sources.size();
    }
    return count;
}

size_t TargetManager::GetHeaderCount() const {
    size_t count = 0;
    for (const auto& [name, config] : m_targets) {
        count += config.headers.size();
    }
    return count;
}

void TargetManager::Clear() {
    m_targets.clear();
    m_buildHistory.clear();
}

Result<void> TargetManager::ValidateTargetConfig(const TargetConfig& config) {
    if (config.name.empty()) {
        return Result<void>::Failure("Target name cannot be empty");
    }
    
    if (config.type == TargetType::Alias && config.aliasFor.empty()) {
        return Result<void>::Failure("Alias target must specify aliasFor");
    }
    
    if (config.type == TargetType::Custom && config.command.empty()) {
        return Result<void>::Failure("Custom target must specify command");
    }
    
    if (config.type != TargetType::Alias && config.type != TargetType::Phony && 
        config.type != TargetType::Custom && config.sources.empty()) {
        return Result<void>::Failure("Target must have sources");
    }
    
    return Result<void>::Success();
}

Path TargetManager::GetTargetOutputPath(const TargetConfig& config) {
    Path outputDir = config.outputDir.empty() ? m_outputDir : config.outputDir;
    if (outputDir.empty()) {
        outputDir = Path("build") / TargetUtils::BuildConfigToString(config.config);
    }
    
    return outputDir / TargetUtils::GetOutputFilename(config);
}

Path TargetManager::GetTargetIntermediatePath(const TargetConfig& config) {
    Path intermediateDir = config.intermediateDir.empty() ? m_intermediateDir : config.intermediateDir;
    if (intermediateDir.empty()) {
        intermediateDir = Path("build") / "intermediate" / TargetUtils::BuildConfigToString(config.config);
    }
    
    return intermediateDir / config.name;
}

Result<void> TargetManager::PrepareBuildDirectories(const TargetConfig& config) {
    IPlatform* platform = PlatformManager::Get().GetPlatform();
    if (!platform) {
        return Result<void>::Failure("Platform not available");
    }
    
    Path outputDir = config.outputDir.empty() ? m_outputDir : config.outputDir;
    if (outputDir.empty()) {
        outputDir = Path("build") / TargetUtils::BuildConfigToString(config.config);
    }
    
    Path intermediateDir = config.intermediateDir.empty() ? m_intermediateDir : config.intermediateDir;
    if (intermediateDir.empty()) {
        intermediateDir = Path("build") / "intermediate" / TargetUtils::BuildConfigToString(config.config);
    }
    
    intermediateDir = intermediateDir / config.name;
    
    if (!platform->DirectoryExists(outputDir)) {
        platform->CreateDirectories(outputDir);
    }
    
    if (!platform->DirectoryExists(intermediateDir)) {
        platform->CreateDirectories(intermediateDir);
    }
    
    return Result<void>::Success();
}

Result<BuildResult> TargetManager::ExecuteBuild(const TargetConfig& config) {
    // Prepare build directories
    auto prepareResult = PrepareBuildDirectories(config);
    if (!prepareResult.IsSuccess()) {
        return Result<BuildResult>::Failure("Failed to prepare build directories: " + prepareResult.GetError());
    }
    
    switch (config.type) {
        case TargetType::Executable:
            return BuildExecutableTarget(config);
        case TargetType::Library:
            return BuildLibraryTarget(config);
        case TargetType::Object:
            return BuildObjectTarget(config);
        case TargetType::Custom:
            return BuildCustomTarget(config);
        case TargetType::Phony:
            // Phony targets always succeed
            BuildResult result;
            result.targetName = config.name;
            result.success = true;
            return Result<BuildResult>::Success(result);
        case TargetType::Alias:
            // Alias targets delegate to their target
            if (!config.aliasFor.empty()) {
                return BuildTarget(config.aliasFor);
            }
            return Result<BuildResult>::Failure("Alias target has no target to alias to");
        default:
            return Result<BuildResult>::Failure("Unknown target type");
    }
}

Result<BuildResult> TargetManager::BuildExecutableTarget(const TargetConfig& config) {
    BuildResult result;
    result.targetName = config.name;
    
    // TODO: Implement actual compilation and linking
    // For now, just return success
    result.success = true;
    result.output = "Built executable " + config.name;
    
    return Result<BuildResult>::Success(result);
}

Result<BuildResult> TargetManager::BuildLibraryTarget(const TargetConfig& config) {
    BuildResult result;
    result.targetName = config.name;
    
    // TODO: Implement actual library compilation
    // For now, just return success
    result.success = true;
    result.output = "Built library " + config.name;
    
    return Result<BuildResult>::Success(result);
}

Result<BuildResult> TargetManager::BuildObjectTarget(const TargetConfig& config) {
    BuildResult result;
    result.targetName = config.name;
    
    // TODO: Implement actual object compilation
    // For now, just return success
    result.success = true;
    result.output = "Built object " + config.name;
    
    return Result<BuildResult>::Success(result);
}

Result<BuildResult> TargetManager::BuildCustomTarget(const TargetConfig& config) {
    BuildResult result;
    result.targetName = config.name;
    
    // TODO: Execute custom command
    // For now, just return success
    result.success = true;
    result.output = "Executed custom target " + config.name;
    
    return Result<BuildResult>::Success(result);
}

// TargetUtils implementation
namespace TargetUtils {

Path GetDefaultOutputDirectory(const TargetConfig& config) {
    return Path("Build") / "Output" / "Win64" / BuildConfigToString(config.config);
}

Path GetDefaultIntermediateDirectory(const TargetConfig& config) {
    return Path("Build") / "Intermediate" / "Win64" / BuildConfigToString(config.config) / config.name;
}

std::string GetOutputFilename(const TargetConfig& config) {
    std::string filename = config.name;
    
    switch (config.type) {
        case TargetType::Executable:
            #ifdef _WIN32
            filename += ".exe";
            #endif
            break;
        case TargetType::Library:
            #ifdef _WIN32
            filename = filename + ".lib";
            #else
            filename = "lib" + filename + ".a";
            #endif
            break;
        case TargetType::Object:
            filename += ".o";
            break;
        default:
            break;
    }
    
    return filename;
}

TargetType DetectTargetType(const TargetConfig& config) {
    return config.type;
}

Result<void> ValidateTargetConfig(const TargetConfig& config) {
    if (config.name.empty()) {
        return Result<void>::Failure("Target name cannot be empty");
    }
    
    return Result<void>::Success();
}

void ResolvePaths(TargetConfig& config, const Path& basePath) {
    if (!config.outputDir.is_absolute()) {
        config.outputDir = basePath / config.outputDir;
    }
    
    if (!config.intermediateDir.is_absolute()) {
        config.intermediateDir = basePath / config.intermediateDir;
    }
}

TargetConfig MergeTargetConfigs(const TargetConfig& base, const TargetConfig& override) {
    TargetConfig merged = base;
    
    if (!override.outputDir.empty()) {
        merged.outputDir = override.outputDir;
    }
    
    if (!override.intermediateDir.empty()) {
        merged.intermediateDir = override.intermediateDir;
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

bool IsDependency(const TargetConfig& target, const TargetConfig& potentialDep) {
    for (const std::string& depName : target.dependencies) {
        if (depName == potentialDep.name) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> GetTransitiveDependencies(const TargetManager& manager,
                                                   const std::string& targetName) {
    std::vector<std::string> dependencies;
    std::unordered_set<std::string> visited;
    std::queue<std::string> queue;
    
    const TargetConfig* target = manager.GetTarget(targetName);
    if (!target) {
        return dependencies;
    }
    
    for (const std::string& dep : target->dependencies) {
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
        
        const TargetConfig* currentTarget = manager.GetTarget(current);
        if (currentTarget) {
            for (const std::string& dep : currentTarget->dependencies) {
                queue.push(dep);
            }
        }
    }
    
    return dependencies;
}

std::string BuildConfigToString(BuildConfig config) {
    switch (config) {
        case BuildConfig::Debug:
            return "Debug";
        case BuildConfig::Release:
            return "Release";
        case BuildConfig::RelWithDebInfo:
            return "RelWithDebInfo";
        case BuildConfig::MinSizeRel:
            return "MinSizeRel";
        default:
            return "Unknown";
    }
}

Result<BuildConfig> ParseBuildConfig(const std::string& config) {
    if (config == "Debug" || config == "debug") {
        return Result<BuildConfig>::Success(BuildConfig::Debug);
    } else if (config == "Release" || config == "release") {
        return Result<BuildConfig>::Success(BuildConfig::Release);
    } else if (config == "RelWithDebInfo" || config == "relwithdebinfo") {
        return Result<BuildConfig>::Success(BuildConfig::RelWithDebInfo);
    } else if (config == "MinSizeRel" || config == "minsizerel") {
        return Result<BuildConfig>::Success(BuildConfig::MinSizeRel);
    }
    
    return Result<BuildConfig>::Failure("Unknown build configuration: " + config);
}

std::vector<std::string> GetDefaultCompileFlags(BuildConfig config) {
    std::vector<std::string> flags;
    
    switch (config) {
        case BuildConfig::Debug:
            flags = { "-g", "-O0", "-DDEBUG" };
            break;
        case BuildConfig::Release:
            flags = { "-O3", "-DNDEBUG" };
            break;
        case BuildConfig::RelWithDebInfo:
            flags = { "-O2", "-g", "-DNDEBUG" };
            break;
        case BuildConfig::MinSizeRel:
            flags = { "-Os", "-DNDEBUG" };
            break;
    }
    
    return flags;
}

std::vector<std::string> GetDefaultLinkFlags(BuildConfig config) {
    std::vector<std::string> flags;
    
    // Add platform-specific link flags
    #ifdef _WIN32
    flags.push_back("/SUBSYSTEM:CONSOLE");
    #else
    flags.push_back("-pthread");
    #endif
    
    return flags;
}

} // namespace TargetUtils

} // namespace IgniteBT
