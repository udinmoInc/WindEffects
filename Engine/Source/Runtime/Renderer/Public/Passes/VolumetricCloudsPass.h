#pragma once

#include "Graph/RenderPass.h"
#include "Graph/FrameContext.h"
#include <volk.h>

namespace we::runtime::renderer {

class DeviceContext;
class GraphicsPipelineFactory;

struct VolumetricCloudsPassConfig {
    DeviceContext* deviceContext = nullptr;
    GraphicsPipelineFactory* pipelineFactory = nullptr;
    VkDescriptorSetLayout cameraDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout environmentDescriptorSetLayout = VK_NULL_HANDLE;
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
};

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
    VolumetricCloudsPassConfig m_Config;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    bool m_Initialized = false;
};

} // namespace we::runtime::renderer
