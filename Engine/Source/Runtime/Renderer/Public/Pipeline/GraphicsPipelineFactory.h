#pragma once

#include "Shader/ShaderTypes.h"
#include <volk.h>
#include <string>
#include <vector>

namespace we::runtime::renderer {

class DeviceContext;

struct GraphicsPipelineDesc {
    std::string vertexShaderName;
    std::string pixelShaderName;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<VkVertexInputBindingDescription> vertexBindings;
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    bool depthTest = false;
    bool depthWrite = false;
    VkCompareOp depthCompare = VK_COMPARE_OP_LESS_OR_EQUAL;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    bool blendEnable = false;
    VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
};

class GraphicsPipelineFactory {
public:
    explicit GraphicsPipelineFactory(DeviceContext* deviceContext);

    GraphicsPipelineFactory(const GraphicsPipelineFactory&) = delete;
    GraphicsPipelineFactory& operator=(const GraphicsPipelineFactory&) = delete;

    VkPipeline CreateGraphicsPipeline(const GraphicsPipelineDesc& desc, VkPipelineLayout& outLayout);
    void DestroyPipeline(VkPipeline pipeline);
    void DestroyPipelineLayout(VkPipelineLayout layout);

private:
    VkShaderModule LoadShaderModule(const std::string& shaderName, ShaderStage stage);

    DeviceContext* m_DeviceContext = nullptr;
};

} // namespace we::runtime::renderer
