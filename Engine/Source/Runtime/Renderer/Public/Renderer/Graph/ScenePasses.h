#pragma once

#include "Renderer/Graph/RenderGraph.h"

namespace we::runtime::renderer {

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
        we::rhi::RHITextureHandle color,
        we::rhi::RHITextureHandle depth,
        we::rhi::Extent2D extent);
    void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) override;
    void Execute(const GraphPassContext& ctx) override;

private:
    we::rhi::RHITextureHandle m_Color = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureHandle m_Depth = we::rhi::RHITextureHandle::Invalid;
    we::rhi::Extent2D m_Extent{};
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
