#pragma once

#include "Camera/CameraUniform.h"
#include "Lighting/SceneEnvironmentUniform.h"
#include "Renderer/Graph/RenderGraph.h"

namespace we::runtime::renderer {

class ViewportSkyRenderer;
class ViewportGridRenderer;

class RENDERER_API ClearPass final : public RenderPass {
public:
    ClearPass(we::rhi::RHITextureHandle target, we::rhi::Color4f color, we::rhi::Extent2D extent);
    void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) override;
    void Execute(const GraphPassContext& ctx) override;

private:
    we::rhi::RHITextureHandle m_Target = we::rhi::RHITextureHandle::Invalid;
    we::rhi::Color4f m_Color{};
    we::rhi::Extent2D m_Extent{};
};

class RENDERER_API ViewportSkyPass final : public RenderPass {
public:
    ViewportSkyPass(
        ViewportSkyRenderer* sky,
        ViewportGridRenderer* grid,
        we::rhi::RHITextureHandle color,
        we::rhi::RHITextureHandle depth,
        we::rhi::Extent2D extent,
        const CameraUniform* camera,
        const SceneEnvironmentUniform* environment);
    void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) override;
    void Execute(const GraphPassContext& ctx) override;

private:
    ViewportSkyRenderer* m_Sky = nullptr;
    ViewportGridRenderer* m_Grid = nullptr;
    we::rhi::RHITextureHandle m_Color = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureHandle m_Depth = we::rhi::RHITextureHandle::Invalid;
    we::rhi::Extent2D m_Extent{};
    const CameraUniform* m_Camera = nullptr;
    const SceneEnvironmentUniform* m_Environment = nullptr;
};

class RENDERER_API TonemapPresentPrepPass final : public RenderPass {
public:
    explicit TonemapPresentPrepPass(we::rhi::RHITextureHandle swapchainImage);
    void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) override;
    void Execute(const GraphPassContext& ctx) override;

private:
    we::rhi::RHITextureHandle m_Swapchain = we::rhi::RHITextureHandle::Invalid;
};

} // namespace we::runtime::renderer
