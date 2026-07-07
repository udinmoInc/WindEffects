#pragma once

#include <cstdint>
#include <unordered_map>

namespace we::runtime::renderer {

struct GraphicsPipelineStateKey {
    uint64_t shaderVSHash = 0;
    uint64_t shaderPSHash = 0;
    uint64_t vertexLayoutHash = 0;
    uint64_t renderPassHash = 0;
    uint32_t permutationFlags = 0;

    bool operator==(const GraphicsPipelineStateKey& other) const;
};

struct GraphicsPipelineStateKeyHash {
    size_t operator()(const GraphicsPipelineStateKey& key) const noexcept;
};

// PSO cache — stores pipeline hashes; Vulkan pipeline objects registered at render time.
class ShaderPipelineCache {
public:
    bool Contains(const GraphicsPipelineStateKey& key) const;
    void Store(const GraphicsPipelineStateKey& key, uint64_t pipelineHandle);
    uint64_t Find(const GraphicsPipelineStateKey& key) const;

    void Clear();

private:
    std::unordered_map<GraphicsPipelineStateKey, uint64_t, GraphicsPipelineStateKeyHash> m_Cache;
};

} // namespace we::runtime::renderer
