#pragma once

#include "Renderer/Shader/ShaderReflection.hpp"
#include "Renderer/Shader/ShaderCompiler.hpp"
#include "Renderer/Shader/ShaderDependencyGraph.hpp"
#include "Renderer/Shader/ShaderPermutationManager.hpp"
#include "Renderer/Shader/ShaderPipelineCache.hpp"
#include "Renderer/Shader/ShaderTypes.hpp"
#include <mutex>
#include <string>
#include <unordered_map>
#include <volk.h>

namespace we::runtime::renderer {

// Central shader library: bytecode loading, hot reload, permutations, PSO cache.
class ShaderLibrary {
public:
    static ShaderLibrary& Get();

    void Initialize(const std::string& shaderRoot, const std::string& bytecodeRoot);
    void Shutdown();

    ShaderBytecode GetBytecode(const std::string& shaderName, ShaderStage stage, uint32_t permutationFlags = 0);
    VkShaderModule CreateShaderModule(VkDevice device, const std::string& shaderName, ShaderStage stage, uint32_t permutationFlags = 0);

    void ReloadShader(const std::string& shaderName);
    void ReloadIfSourcesChanged();

    ShaderPipelineCache& GetPipelineCache() { return m_PipelineCache; }
    ShaderDependencyGraph& GetDependencyGraph() { return m_DependencyGraph; }
    ShaderPermutationManager& GetPermutationManager() { return m_PermutationManager; }

    std::string ResolveBytecodeFilename(const std::string& shaderName, ShaderStage stage, uint32_t permutationFlags = 0) const;

private:
    ShaderLibrary() = default;

    ShaderPermutationKey MakeKey(const std::string& shaderName, ShaderStage stage, uint32_t permutationFlags) const;
    std::string StageSuffix(ShaderStage stage) const;

    std::mutex m_Mutex;
    bool m_Initialized = false;
    std::string m_ShaderRoot;
    std::string m_BytecodeRoot;
    ShaderCompiler m_Compiler;
    ShaderDependencyGraph m_DependencyGraph;
    ShaderPermutationManager m_PermutationManager;
    ShaderPipelineCache m_PipelineCache;
    std::unordered_map<std::string, ShaderBytecode> m_BytecodeCache;
};

} // namespace we::runtime::renderer
