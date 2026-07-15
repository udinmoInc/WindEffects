#pragma once

#include <volk.h>


#pragma warning(push)
#pragma warning(disable: 4251)

#include "Renderer/Export.h"
#include <vector>
#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif

namespace we::runtime::renderer {

class DeviceContext;
class ResourceManager;

struct DirectionalLightData {
#if WE_HAS_GLM
    // Travel direction (from sun toward scene). Sky uses -direction as sun vector.
    glm::vec3 direction{0.3f, -0.8f, 0.2f};
    float intensity = 1.2f;
    glm::vec3 color{1.0f, 0.98f, 0.95f};
    float padding = 0.0f;
#else
    float direction[3]{0.3f, -0.8f, 0.2f};
    float intensity = 1.2f;
    float color[3]{1.0f, 0.98f, 0.95f};
    float padding = 0.0f;
#endif
};

struct DirectionalLightConfig {
    DeviceContext* deviceContext = nullptr;
    ResourceManager* resourceManager = nullptr;
    uint32_t maxFramesInFlight = 2;
};

class RENDERER_API DirectionalLight {
public:
    DirectionalLight() = default;
    ~DirectionalLight();

    DirectionalLight(const DirectionalLight&) = delete;
    DirectionalLight& operator=(const DirectionalLight&) = delete;

    void Init(const DirectionalLightConfig& config);
    void Shutdown();

    void Upload(uint32_t frameIndex, const DirectionalLightData& light);
    const DirectionalLightData& GetData(uint32_t frameIndex) const;

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
    std::vector<DirectionalLightData> m_Data;

    bool m_Initialized = false;
};

} // namespace we::runtime::renderer

#pragma warning(pop)
