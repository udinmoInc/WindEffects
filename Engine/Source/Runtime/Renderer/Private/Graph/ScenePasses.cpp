#include "Renderer/Graph/ScenePasses.h"
#include "Graph/ViewportSkyRenderer.h"
#include "Graph/ViewportGridRenderer.h"

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
    ViewportSkyRenderer* sky,
    ViewportGridRenderer* grid,
    we::rhi::RHITextureHandle color,
    we::rhi::RHITextureHandle depth,
    we::rhi::Extent2D extent,
    const CameraUniform* camera,
    const SceneEnvironmentUniform* environment)
    : RenderPass("ViewportSkyPass")
    , m_Sky(sky)
    , m_Grid(grid)
    , m_Color(color)
    , m_Depth(depth)
    , m_Extent(extent)
    , m_Camera(camera)
    , m_Environment(environment)
{
}

void ViewportSkyPass::Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>&) {
    textures.push_back({m_Color, we::rhi::ResourceState::RenderTarget, GraphResourceAccess::Write});
    if (m_Depth != we::rhi::RHITextureHandle::Invalid) {
        textures.push_back({m_Depth, we::rhi::ResourceState::DepthWrite, GraphResourceAccess::Write});
    }
}

void ViewportSkyPass::Execute(const GraphPassContext& ctx) {
    if (!ctx.commandList || !m_Camera || !m_Environment) {
        return;
    }
    if (m_Sky) {
        m_Sky->Draw(*ctx.commandList, m_Color, m_Depth, m_Extent, *m_Camera, *m_Environment);
    }
    if (m_Grid) {
        m_Grid->Draw(*ctx.commandList, m_Color, m_Depth, m_Extent, *m_Camera);
    }
    if (m_Color != we::rhi::RHITextureHandle::Invalid) {
        ctx.commandList->TransitionTexture(
            m_Color,
            we::rhi::ResourceState::RenderTarget,
            we::rhi::ResourceState::ShaderResource);
    }
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
