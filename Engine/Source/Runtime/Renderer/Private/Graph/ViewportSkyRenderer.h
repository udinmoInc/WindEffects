#pragma once

#include "Camera/CameraUniform.h"
#include "Lighting/SceneEnvironmentUniform.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>
#include <vector>

namespace we::runtime::renderer {

// Fullscreen procedural sky into the viewport color target (camera-driven).
class ViewportSkyRenderer {
public:
    ViewportSkyRenderer() = default;
    ~ViewportSkyRenderer();

    ViewportSkyRenderer(const ViewportSkyRenderer&) = delete;
    ViewportSkyRenderer& operator=(const ViewportSkyRenderer&) = delete;

    bool Init(we::rhi::IRHIDevice* device);
    void Shutdown();

    void Draw(
        we::rhi::IRHICommandList& cmd,
        we::rhi::RHITextureHandle color,
        we::rhi::RHITextureHandle depth,
        we::rhi::Extent2D extent,
        const CameraUniform& camera,
        const SceneEnvironmentUniform& environment);

    [[nodiscard]] bool IsReady() const { return m_Ready; }

private:
    bool EnsurePipeline(we::rhi::Format colorFormat, we::rhi::Format depthFormat, bool hasDepth);
    bool LoadShaders();

    we::rhi::IRHIDevice* m_Device = nullptr;
    we::rhi::RHIShaderHandle m_Vs = we::rhi::RHIShaderHandle::Invalid;
    we::rhi::RHIShaderHandle m_Ps = we::rhi::RHIShaderHandle::Invalid;
    std::vector<uint8_t> m_VsBytes;
    std::vector<uint8_t> m_PsBytes;
    we::rhi::RHIDescriptorSetLayoutHandle m_CameraLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    we::rhi::RHIDescriptorSetLayoutHandle m_EnvLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    we::rhi::RHIPipelineLayoutHandle m_PipelineLayout = we::rhi::RHIPipelineLayoutHandle::Invalid;
    we::rhi::RHIGraphicsPipelineHandle m_Pipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;
    we::rhi::RHIDescriptorPoolHandle m_Pool = we::rhi::RHIDescriptorPoolHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_CameraSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_EnvSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    we::rhi::RHIBufferHandle m_CameraBuffer = we::rhi::RHIBufferHandle::Invalid;
    we::rhi::RHIBufferHandle m_EnvBuffer = we::rhi::RHIBufferHandle::Invalid;

    we::rhi::Format m_PipelineColorFormat = we::rhi::Format::Unknown;
    we::rhi::Format m_PipelineDepthFormat = we::rhi::Format::Unknown;
    bool m_PipelineHasDepth = false;
    bool m_Ready = false;
};

} // namespace we::runtime::renderer
