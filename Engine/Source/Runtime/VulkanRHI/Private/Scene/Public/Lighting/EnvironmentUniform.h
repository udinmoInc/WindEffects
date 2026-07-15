#pragma once

#include <volk.h>


#pragma warning(push)
#pragma warning(disable: 4251)

#include "Lighting/SceneEnvironmentUniform.h"
#include "Renderer/Export.h"
#include <vector>

namespace we::runtime::renderer {

class DeviceContext;
class ResourceManager;

struct EnvironmentUniformConfig {
    DeviceContext* deviceContext = nullptr;
    ResourceManager* resourceManager = nullptr;
    uint32_t maxFramesInFlight = 2;
};

class RENDERER_API EnvironmentUniform {
public:
    EnvironmentUniform() = default;
    ~EnvironmentUniform();

    EnvironmentUniform(const EnvironmentUniform&) = delete;
    EnvironmentUniform& operator=(const EnvironmentUniform&) = delete;

    void Init(const EnvironmentUniformConfig& config);
    void Shutdown();

    void Upload(uint32_t frameIndex, const SceneEnvironmentUniform& uniform);
    const SceneEnvironmentUniform& GetData(uint32_t frameIndex) const;

    VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
    VkDescriptorSet GetDescriptorSet(uint32_t frameIndex) const;

private:
    void CreateDescriptorLayout();
    void CreateBuffers(uint32_t frameCount);
    void CreateDescriptorSets(uint32_t frameCount);

    DeviceContext* m_DeviceContext = nullptr;
    ResourceManager* m_ResourceManager = nullptr;

    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

    std::vector<VkBuffer> m_Buffers;
    std::vector<VkDeviceMemory> m_BufferMemories;
    std::vector<VkDescriptorSet> m_DescriptorSets;
    std::vector<SceneEnvironmentUniform> m_Data;

    bool m_Initialized = false;
};

} // namespace we::runtime::renderer

#pragma warning(pop)
