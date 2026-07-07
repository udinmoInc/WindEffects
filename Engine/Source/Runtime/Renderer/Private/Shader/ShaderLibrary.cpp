#include "Shader/ShaderLibrary.h"
#include <filesystem>
#include <sstream>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

ShaderLibrary& ShaderLibrary::Get()
{
    static ShaderLibrary instance;
    return instance;
}

void ShaderLibrary::Initialize(const std::string& shaderRoot, const std::string& bytecodeRoot)
{
    std::lock_guard lock(m_Mutex);
    m_ShaderRoot = shaderRoot;
    m_BytecodeRoot = bytecodeRoot;
    m_BytecodeCache.clear();
    m_PipelineCache.Clear();
    m_DependencyGraph.ScanIncludeTree(shaderRoot);
    m_Initialized = true;
}

void ShaderLibrary::Shutdown()
{
    std::lock_guard lock(m_Mutex);
    m_BytecodeCache.clear();
    m_PipelineCache.Clear();
    m_Initialized = false;
}

std::string ShaderLibrary::StageSuffix(ShaderStage stage) const
{
    switch (stage)
    {
    case ShaderStage::Vertex: return "_VS";
    case ShaderStage::Pixel: return "_PS";
    case ShaderStage::Compute: return "_CS";
    default: return "_SH";
    }
}

std::string ShaderLibrary::ResolveBytecodeFilename(const std::string& shaderName, ShaderStage stage, uint32_t permutationFlags) const
{
    const std::string suffix = m_PermutationManager.BuildPermutationSuffix(permutationFlags);
    return shaderName + StageSuffix(stage) + suffix + ".spv";
}

ShaderPermutationKey ShaderLibrary::MakeKey(const std::string& shaderName, ShaderStage stage, uint32_t permutationFlags) const
{
    ShaderPermutationKey key;
    key.shaderName = shaderName;
    key.stage = stage;
    key.permutationFlags = permutationFlags;
    return key;
}

ShaderBytecode ShaderLibrary::GetBytecode(const std::string& shaderName, ShaderStage stage, uint32_t permutationFlags)
{
    std::lock_guard lock(m_Mutex);

    const std::string filename = ResolveBytecodeFilename(shaderName, stage, permutationFlags);
    const std::string cacheKey = filename;

    const auto cached = m_BytecodeCache.find(cacheKey);
    if (cached != m_BytecodeCache.end() && !cached->second.data.empty())
        return cached->second;

    std::vector<std::string> searchRoots = { m_BytecodeRoot };
    if (!m_BytecodeRoot.empty())
    {
        searchRoots.emplace_back("Engine/Shaders/Bytecodes");
        searchRoots.emplace_back("Assets/Shaders");
        searchRoots.emplace_back("Shaders");
        searchRoots.emplace_back("../Assets/Shaders");
        searchRoots.emplace_back("../Shaders");
    }

    for (const auto& root : searchRoots)
    {
        if (root.empty())
            continue;

        const std::filesystem::path diskPath = std::filesystem::path(root) / filename;
        ShaderBytecode bytecode = m_Compiler.LoadSPIRVFromDisk(diskPath.string());
        if (!bytecode.data.empty())
        {
            m_BytecodeCache[cacheKey] = bytecode;
            return bytecode;
        }
    }

    return {};
}

VkShaderModule ShaderLibrary::CreateShaderModule(VkDevice device, const std::string& shaderName, ShaderStage stage, uint32_t permutationFlags)
{
    const ShaderBytecode bytecode = GetBytecode(shaderName, stage, permutationFlags);
    if (bytecode.data.empty())
        return VK_NULL_HANDLE;

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bytecode.data.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data.data());

    VkShaderModule module = VK_NULL_HANDLE;
    vkCreateShaderModule(device, &createInfo, nullptr, &module);
    return module;
}

void ShaderLibrary::ReloadShader(const std::string& shaderName)
{
    std::lock_guard lock(m_Mutex);
    for (auto it = m_BytecodeCache.begin(); it != m_BytecodeCache.end();)
    {
        if (it->first.rfind(shaderName, 0) == 0)
            it = m_BytecodeCache.erase(it);
        else
            ++it;
    }
}

void ShaderLibrary::ReloadIfSourcesChanged()
{
    if (!m_Initialized)
        return;

    std::lock_guard lock(m_Mutex);

    const std::filesystem::path root(m_ShaderRoot);
    if (!std::filesystem::exists(root))
        return;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".hlsl")
            continue;

        const std::string shaderPath = entry.path().generic_string();
        const std::string shaderName = entry.path().stem().string();
        const uint64_t latest = m_DependencyGraph.GetLatestTimestamp(shaderPath);

        for (ShaderStage stage : {ShaderStage::Vertex, ShaderStage::Pixel})
        {
            const std::string cacheKey = ResolveBytecodeFilename(shaderName, stage, 0);
            const auto it = m_BytecodeCache.find(cacheKey);
            if (it != m_BytecodeCache.end() && it->second.sourceTimestamp < latest)
                m_BytecodeCache.erase(it);
        }
    }
}

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
