#include "Renderer/Graph/ScenePasses.h"
#include "Graph/ViewportSkyRenderer.h"
#include "Graph/ViewportGridRenderer.h"

namespace we::runtime::renderer {

EnvUploadPass::EnvUploadPass(
    ViewportSkyRenderer* sky,
    const CameraUniform* camera,
    const SceneEnvironmentUniform* environment)
    : RenderPass("EnvUploadPass", we::rhi::QueueType::Transfer, GraphPassFlags::SideEffects)
    , m_Sky(sky)
    , m_Camera(camera)
    , m_Environment(environment)
{
}

void EnvUploadPass::Setup(std::vector<GraphTextureRef>&, std::vector<GraphBufferRef>& buffers) {
    if (!m_Sky) {
        return;
    }
    const auto env = m_Sky->GetEnvironmentBuffer();
    if (env != we::rhi::RHIBufferHandle::Invalid) {
        buffers.push_back({kInvalidGraphResourceId, env, we::rhi::ResourceState::CopyDst, GraphResourceAccess::Write});
    }
    const auto cam = m_Sky->GetCameraBuffer();
    if (cam != we::rhi::RHIBufferHandle::Invalid) {
        buffers.push_back({kInvalidGraphResourceId, cam, we::rhi::ResourceState::CopyDst, GraphResourceAccess::Write});
    }
}

void EnvUploadPass::Execute(const GraphPassContext&) {
    if (!m_Sky || !m_Camera || !m_Environment) {
        return;
    }
    m_Sky->UploadFrameUniforms(*m_Camera, *m_Environment);
}

ClearPass::ClearPass(we::rhi::RHITextureHandle target, we::rhi::Color4f color, we::rhi::Extent2D extent)
    : RenderPass("ClearPass", we::rhi::QueueType::Graphics, GraphPassFlags::SideEffects)
    , m_Target(target)
    , m_Color(color)
    , m_Extent(extent)
{
}

void ClearPass::Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>&) {
    textures.push_back({kInvalidGraphResourceId, m_Target, we::rhi::ResourceState::RenderTarget, GraphResourceAccess::Write});
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

SkyPass::SkyPass(
    ViewportSkyRenderer* sky,
    we::rhi::RHITextureHandle color,
    we::rhi::RHITextureHandle depth,
    we::rhi::Extent2D extent,
    const CameraUniform* camera,
    const SceneEnvironmentUniform* environment)
    : RenderPass("SkyPass", we::rhi::QueueType::Graphics, GraphPassFlags::SideEffects)
    , m_Sky(sky)
    , m_Color(color)
    , m_Depth(depth)
    , m_Extent(extent)
    , m_Camera(camera)
    , m_Environment(environment)
{
}

void SkyPass::Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>&) {
    textures.push_back({kInvalidGraphResourceId, m_Color, we::rhi::ResourceState::RenderTarget, GraphResourceAccess::Write});
    if (m_Depth != we::rhi::RHITextureHandle::Invalid) {
        textures.push_back({kInvalidGraphResourceId, m_Depth, we::rhi::ResourceState::DepthWrite, GraphResourceAccess::Write});
    }
}

void SkyPass::Execute(const GraphPassContext& ctx) {
    if (!ctx.commandList || !m_Camera || !m_Environment || !m_Sky) {
        return;
    }
    m_Sky->Draw(*ctx.commandList, m_Color, m_Depth, m_Extent, *m_Camera, *m_Environment);
}

GridPass::GridPass(
    ViewportGridRenderer* grid,
    we::rhi::RHITextureHandle color,
    we::rhi::RHITextureHandle depth,
    we::rhi::Extent2D extent,
    const CameraUniform* camera)
    : RenderPass("GridPass", we::rhi::QueueType::Graphics, GraphPassFlags::SideEffects)
    , m_Grid(grid)
    , m_Color(color)
    , m_Depth(depth)
    , m_Extent(extent)
    , m_Camera(camera)
{
}

void GridPass::Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>&) {
    textures.push_back({kInvalidGraphResourceId, m_Color, we::rhi::ResourceState::RenderTarget, GraphResourceAccess::ReadWrite});
    if (m_Depth != we::rhi::RHITextureHandle::Invalid) {
        textures.push_back({kInvalidGraphResourceId, m_Depth, we::rhi::ResourceState::DepthWrite, GraphResourceAccess::ReadWrite});
    }
}

void GridPass::Execute(const GraphPassContext& ctx) {
    if (!ctx.commandList || !m_Camera || !m_Grid) {
        return;
    }
    m_Grid->Draw(*ctx.commandList, m_Color, m_Depth, m_Extent, *m_Camera);
    if (m_Color != we::rhi::RHITextureHandle::Invalid) {
        ctx.commandList->TransitionTexture(
            m_Color,
            we::rhi::ResourceState::RenderTarget,
            we::rhi::ResourceState::ShaderResource);
    }
}

TonemapPass::TonemapPass(we::rhi::RHITextureHandle hdrColor, we::rhi::RHITextureHandle swapchainImage)
    : RenderPass("TonemapPass", we::rhi::QueueType::Graphics, GraphPassFlags::KeepAlive)
    , m_Hdr(hdrColor)
    , m_Swapchain(swapchainImage)
{
}

void TonemapPass::Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>&) {
    if (m_Hdr != we::rhi::RHITextureHandle::Invalid) {
        textures.push_back({kInvalidGraphResourceId, m_Hdr, we::rhi::ResourceState::ShaderResource, GraphResourceAccess::Read});
    }
    textures.push_back({kInvalidGraphResourceId, m_Swapchain, we::rhi::ResourceState::RenderTarget, GraphResourceAccess::ReadWrite});
}

void TonemapPass::Execute(const GraphPassContext&) {
    // HDR resolve / ACES tonemap will bind Engine/Shaders/PostProcess here.
    // Viewport is sampled by UI; chrome+UI composite onto swapchain today.
}

UiOverlayPass::UiOverlayPass(we::rhi::RHITextureHandle swapchainImage, OverlayRecordFn recorder)
    : RenderPass("UiOverlayPass", we::rhi::QueueType::Graphics, GraphPassFlags::SideEffects)
    , m_Swapchain(swapchainImage)
    , m_Recorder(std::move(recorder))
{
}

void UiOverlayPass::Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>&) {
    textures.push_back({kInvalidGraphResourceId, m_Swapchain, we::rhi::ResourceState::RenderTarget, GraphResourceAccess::ReadWrite});
}

void UiOverlayPass::Execute(const GraphPassContext& ctx) {
    if (m_Recorder) {
        m_Recorder(ctx, m_Swapchain);
    }
}

PresentPass::PresentPass(we::rhi::RHITextureHandle swapchainImage)
    : RenderPass("PresentPass", we::rhi::QueueType::Present, GraphPassFlags::SideEffects)
    , m_Swapchain(swapchainImage)
{
}

void PresentPass::Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>&) {
    textures.push_back({kInvalidGraphResourceId, m_Swapchain, we::rhi::ResourceState::Present, GraphResourceAccess::Read});
}

void PresentPass::Execute(const GraphPassContext&) {
    // Barrier to Present is emitted by Setup; Renderer::SubmitAndPresent submits.
}

StubGraphicsPass::StubGraphicsPass(
    std::string name,
    uint32_t writeTextureId,
    we::rhi::ResourceState writeState,
    uint32_t readTextureId,
    we::rhi::ResourceState readState)
    : RenderPass(std::move(name), we::rhi::QueueType::Graphics, GraphPassFlags::KeepAlive)
    , m_WriteId(writeTextureId)
    , m_ReadId(readTextureId)
    , m_WriteState(writeState)
    , m_ReadState(readState)
{
}

void StubGraphicsPass::Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>&) {
    if (m_ReadId != kInvalidGraphResourceId) {
        textures.push_back({m_ReadId, we::rhi::RHITextureHandle::Invalid, m_ReadState, GraphResourceAccess::Read});
    }
    if (m_WriteId != kInvalidGraphResourceId) {
        textures.push_back({m_WriteId, we::rhi::RHITextureHandle::Invalid, m_WriteState, GraphResourceAccess::Write});
    }
}

void StubGraphicsPass::Execute(const GraphPassContext&) {}

StubComputePass::StubComputePass(std::string name, uint32_t writeTextureId, uint32_t readTextureId)
    : RenderPass(
          std::move(name),
          we::rhi::QueueType::Compute,
          GraphPassFlags::KeepAlive | GraphPassFlags::AsyncCapable)
    , m_WriteId(writeTextureId)
    , m_ReadId(readTextureId)
{
}

void StubComputePass::Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>&) {
    if (m_ReadId != kInvalidGraphResourceId) {
        textures.push_back({
            m_ReadId,
            we::rhi::RHITextureHandle::Invalid,
            we::rhi::ResourceState::ShaderResource,
            GraphResourceAccess::Read});
    }
    if (m_WriteId != kInvalidGraphResourceId) {
        textures.push_back({
            m_WriteId,
            we::rhi::RHITextureHandle::Invalid,
            we::rhi::ResourceState::UnorderedAccess,
            GraphResourceAccess::Write});
    }
}

void StubComputePass::Execute(const GraphPassContext&) {}

} // namespace we::runtime::renderer
