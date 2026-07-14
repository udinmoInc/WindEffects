#pragma once

#include <volk.h>
#include <array>
#include <cstdint>
#include <vector>
#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif

namespace we::runtime::renderer {

class DeviceContext;
class ResourceManager;

struct CameraUniform {
#if WE_HAS_GLM
    glm::mat4 view{};
    glm::mat4 proj{};
    glm::mat4 invViewProj{};
    glm::vec3 position{};
#else
    float view[16]{};
    float proj[16]{};
    float invViewProj[16]{};
    float position[3]{};
#endif
    // Foundation sky debug: 0=final, 1=sky only, 2=sun mask, 3=luminance, 4=no sun, 5=linear HDR.
    float padding = 0.0f;
};

struct CameraSystemConfig {
    DeviceContext* deviceContext = nullptr;
    ResourceManager* resourceManager = nullptr;
    uint32_t maxFramesInFlight = 2;
};

class CameraSystem {
public:
    CameraSystem() = default;
    ~CameraSystem();

    CameraSystem(const CameraSystem&) = delete;
    CameraSystem& operator=(const CameraSystem&) = delete;

    void Init(const CameraSystemConfig& config);
    void Shutdown();

    void Upload(uint32_t frameIndex, const CameraUniform& uniform);
    const CameraUniform& GetLastUploaded(uint32_t frameIndex) const;

    VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
    VkDescriptorSet GetDescriptorSet(uint32_t frameIndex) const;
    VkBuffer GetBuffer(uint32_t frameIndex) const;

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
    std::vector<CameraUniform> m_LastUploaded;

    bool m_Initialized = false;
};

} // namespace we::runtime::renderer
