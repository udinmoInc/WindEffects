#pragma once

#include "Graph/RenderPass.h"
#include "Graph/FrameContext.h"
#include <volk.h>

namespace we::runtime::renderer {

class DeviceContext;
class GraphicsPipelineFactory;
class SceneRenderer;

struct PBRPassConfig {
    DeviceContext* deviceContext = nullptr;
    GraphicsPipelineFactory* pipelineFactory = nullptr;
    VkDescriptorSetLayout cameraDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout lightDescriptorSetLayout = VK_NULL_HANDLE;
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
};

class PBRPass : public RenderPass {
public:
    PBRPass();
    ~PBRPass() override;

    void Init(const PBRPassConfig& config);
    void Shutdown();

    void Validate() override;
    void Execute(const FrameContext& frame) override;
    RenderPassIO DescribePassIO() const override;

private:
    PBRPassConfig m_Config;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    bool m_Initialized = false;
};

} // namespace we::runtime::renderer
