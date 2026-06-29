#include "Renderer/Shader/ShaderPipelineCache.hpp"
#include <functional>

namespace we::runtime::renderer {

bool GraphicsPipelineStateKey::operator==(const GraphicsPipelineStateKey& other) const
{
    return shaderVSHash == other.shaderVSHash
        && shaderPSHash == other.shaderPSHash
        && vertexLayoutHash == other.vertexLayoutHash
        && renderPassHash == other.renderPassHash
        && permutationFlags == other.permutationFlags;
}

size_t GraphicsPipelineStateKeyHash::operator()(const GraphicsPipelineStateKey& key) const noexcept
{
    size_t h = 0;
    auto mix = [&h](uint64_t v) {
        h ^= std::hash<uint64_t>{}(v) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    };
    mix(key.shaderVSHash);
    mix(key.shaderPSHash);
    mix(key.vertexLayoutHash);
    mix(key.renderPassHash);
    mix(key.permutationFlags);
    return h;
}

bool ShaderPipelineCache::Contains(const GraphicsPipelineStateKey& key) const
{
    return m_Cache.find(key) != m_Cache.end();
}

void ShaderPipelineCache::Store(const GraphicsPipelineStateKey& key, uint64_t pipelineHandle)
{
    m_Cache[key] = pipelineHandle;
}

uint64_t ShaderPipelineCache::Find(const GraphicsPipelineStateKey& key) const
{
    const auto it = m_Cache.find(key);
    return it != m_Cache.end() ? it->second : 0;
}

void ShaderPipelineCache::Clear()
{
    m_Cache.clear();
}

} // namespace we::runtime::renderer
