#pragma once

#include "Graph/RenderPass.h"
#include "Graph/FrameContext.h"
#include <volk.h>

namespace we::runtime::renderer {

class DeviceContext;
class GraphicsPipelineFactory;

struct SkyPassConfig {
    DeviceContext* deviceContext = nullptr;
    GraphicsPipelineFactory* pipelineFactory = nullptr;
    VkDescriptorSetLayout cameraDescriptorSetLayout = VK_NULL_HANDLE;
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
};

class SkyPass : public RenderPass {
public:
    SkyPass();
    ~SkyPass() override;

    void Init(const SkyPassConfig& config);
    void Shutdown();

    void Validate() override;
    void Execute(const FrameContext& frame) override;

private:
    SkyPassConfig m_Config;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    bool m_Initialized = false;
};

} // namespace we::runtime::renderer
