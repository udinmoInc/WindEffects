#pragma once

#include <volk.h>


#include "Graph/RenderPass.h"
#include "Graph/FrameContext.h"
#include "Lighting/CloudRenderTargets.h"
#include <cstdint>
#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif

namespace we::runtime::renderer {

class DeviceContext;
class GraphicsPipelineFactory;
class ResourceManager;

struct VolumetricCloudsPassConfig {
    DeviceContext* deviceContext = nullptr;
    GraphicsPipelineFactory* pipelineFactory = nullptr;
    ResourceManager* resourceManager = nullptr;
    VkDescriptorSetLayout cameraDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout environmentDescriptorSetLayout = VK_NULL_HANDLE;
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
    float resolutionScale = 0.5f;
};

// Multi-stage volumetric clouds:
//   1) half-res world-space raymarch → scratch
//   2) temporal resolve with reprojection → history ping-pong
//   3) bilateral upsample + composite → SceneColor
class VolumetricCloudsPass : public RenderPass {
public:
    VolumetricCloudsPass();
    ~VolumetricCloudsPass() override;

    void Init(const VolumetricCloudsPassConfig& config);
    void Shutdown();

    void Validate() override;
    void Execute(const FrameContext& frame) override;
    RenderPassIO DescribePassIO() const override;

private:
    void EnsureTargets(uint32_t fullWidth, uint32_t fullHeight);
    void CreateCloudDescriptorLayout();
    void CreateCloudDescriptorSets();
    void UpdateCloudDescriptors(VkDescriptorSet set, VkImageView image1, VkImageView image2, VkImageView depthView);
    void UploadCloudFrame(uint32_t frameIndex, const FrameContext& frame, bool historyValid);

    VolumetricCloudsPassConfig m_Config{};
    CloudRenderTargets m_Targets;

    VkPipeline m_RaymarchPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_RaymarchLayout = VK_NULL_HANDLE;
    VkPipeline m_TemporalPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_TemporalLayout = VK_NULL_HANDLE;
    VkPipeline m_CompositePipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_CompositeLayout = VK_NULL_HANDLE;

    VkDescriptorSetLayout m_CloudSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_CloudPool = VK_NULL_HANDLE;
    VkDescriptorSet m_CloudSets[2]{}; // ping-pong: [writeIndex]

    VkBuffer m_CloudFrameBuffers[2]{};
    VkDeviceMemory m_CloudFrameMemories[2]{};

#if WE_HAS_GLM
    glm::mat4 m_PrevViewProj{1.0f};
    glm::vec3 m_PrevCameraPos{0.0f};
#endif
    bool m_HasHistory = false;
    uint32_t m_HistoryWrite = 0;
    uint32_t m_FrameCounter = 0;
    float m_ResolutionScale = 0.5f;
    bool m_Initialized = false;
};

} // namespace we::runtime::renderer
