#pragma once

#include "Camera/CameraUniform.h"
#include <volk.h>

#include <array>
#include <cstdint>
#include <vector>

namespace we::runtime::renderer {

class DeviceContext;
class ResourceManager;

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
