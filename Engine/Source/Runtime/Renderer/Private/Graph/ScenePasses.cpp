#include "Renderer/Graph/ScenePasses.h"

namespace we::runtime::renderer {

ClearPass::ClearPass(we::rhi::RHITextureHandle target, we::rhi::Color4f color, we::rhi::Extent2D extent)
    : RenderPass("ClearPass")
    , m_Target(target)
    , m_Color(color)
    , m_Extent(extent)
{
}

void ClearPass::Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>&) {
    textures.push_back({m_Target, we::rhi::ResourceState::RenderTarget, GraphResourceAccess::Write});
}

void ClearPass::Execute(const GraphPassContext& ctx) {
    if (!ctx.commandList || m_Target == we::rhi::RHITextureHandle::Invalid) {
        return;
    }
    we::rhi::RenderingInfo info{};
    we::rhi::ColorAttachmentDesc color{};
    color.texture = m_Target;
    color.loadOp = we::rhi::LoadOp::Clear;
    color.storeOp = we::rhi::StoreOp::Store;
    color.clearColor = m_Color;
    info.colorAttachments.push_back(color);
    info.renderArea = m_Extent;
    ctx.commandList->BeginRendering(info);
    ctx.commandList->SetViewport({0, 0, static_cast<float>(m_Extent.width), static_cast<float>(m_Extent.height), 0, 1});
    ctx.commandList->SetScissor({0, 0, m_Extent.width, m_Extent.height});
    ctx.commandList->EndRendering();
}

ViewportSkyPass::ViewportSkyPass(
    we::rhi::RHITextureHandle color,
    we::rhi::RHITextureHandle depth,
    we::rhi::Extent2D extent)
    : RenderPass("ViewportSkyPass")
    , m_Color(color)
    , m_Depth(depth)
    , m_Extent(extent)
{
}

void ViewportSkyPass::Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>&) {
    textures.push_back({m_Color, we::rhi::ResourceState::RenderTarget, GraphResourceAccess::Write});
    if (m_Depth != we::rhi::RHITextureHandle::Invalid) {
        textures.push_back({m_Depth, we::rhi::ResourceState::DepthWrite, GraphResourceAccess::Write});
    }
}

void ViewportSkyPass::Execute(const GraphPassContext& ctx) {
    if (!ctx.commandList || m_Color == we::rhi::RHITextureHandle::Invalid) {
        return;
    }
    we::rhi::RenderingInfo info{};
    we::rhi::ColorAttachmentDesc color{};
    color.texture = m_Color;
    color.loadOp = we::rhi::LoadOp::Clear;
    color.storeOp = we::rhi::StoreOp::Store;
    color.clearColor = {0.45f, 0.62f, 0.92f, 1.0f};
    info.colorAttachments.push_back(color);
    if (m_Depth != we::rhi::RHITextureHandle::Invalid) {
        info.depth.enabled = true;
        info.depth.texture = m_Depth;
        info.depth.loadOp = we::rhi::LoadOp::Clear;
        info.depth.storeOp = we::rhi::StoreOp::Store;
        info.depth.clearDepth = 1.0f;
    }
    info.renderArea = m_Extent;
    ctx.commandList->BeginRendering(info);
    ctx.commandList->SetViewport({0, 0, static_cast<float>(m_Extent.width), static_cast<float>(m_Extent.height), 0, 1});
    ctx.commandList->SetScissor({0, 0, m_Extent.width, m_Extent.height});
    ctx.commandList->EndRendering();
    ctx.commandList->TransitionTexture(
        m_Color,
        we::rhi::ResourceState::RenderTarget,
        we::rhi::ResourceState::ShaderResource);
}

TonemapPresentPrepPass::TonemapPresentPrepPass(we::rhi::RHITextureHandle swapchainImage)
    : RenderPass("TonemapPresentPrepPass")
    , m_Swapchain(swapchainImage)
{
}

void TonemapPresentPrepPass::Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>&) {
    textures.push_back({m_Swapchain, we::rhi::ResourceState::RenderTarget, GraphResourceAccess::ReadWrite});
}

void TonemapPresentPrepPass::Execute(const GraphPassContext&) {
    // Full HDR resolve / ACES tonemap compute/graphics pass will bind existing
    // Engine/Shaders/PostProcess assets here; chrome+UI currently composite onto swapchain.
}

} // namespace we::runtime::renderer
