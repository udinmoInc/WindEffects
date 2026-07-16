#pragma once

#pragma warning(push)
#pragma warning(disable : 4251)

#include "Camera/CameraUniform.h"
#include "Lighting/SceneEnvironmentUniform.h"
#include "Renderer/Graph/RenderGraph.h"
#include "ECS/RenderExtract.h"

#include <functional>

namespace we::runtime::renderer {

class ViewportSkyRenderer;
class ViewportGridRenderer;

// --- Running / migrated ----------------------------------------------------

class RENDERER_API EnvUploadPass final : public RenderPass {
public:
    EnvUploadPass(
        ViewportSkyRenderer* sky,
        const CameraUniform* camera,
        const SceneEnvironmentUniform* environment);
    void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) override;
    void Execute(const GraphPassContext& ctx) override;

private:
    ViewportSkyRenderer* m_Sky = nullptr;
    const CameraUniform* m_Camera = nullptr;
    const SceneEnvironmentUniform* m_Environment = nullptr;
};

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

class RENDERER_API SkyPass final : public RenderPass {
public:
    SkyPass(
        ViewportSkyRenderer* sky,
        we::rhi::RHITextureHandle color,
        we::rhi::RHITextureHandle depth,
        we::rhi::Extent2D extent,
        const CameraUniform* camera,
        const SceneEnvironmentUniform* environment);
    void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) override;
    void Execute(const GraphPassContext& ctx) override;

private:
    ViewportSkyRenderer* m_Sky = nullptr;
    we::rhi::RHITextureHandle m_Color = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureHandle m_Depth = we::rhi::RHITextureHandle::Invalid;
    we::rhi::Extent2D m_Extent{};
    const CameraUniform* m_Camera = nullptr;
    const SceneEnvironmentUniform* m_Environment = nullptr;
};

class RENDERER_API GridPass final : public RenderPass {
public:
    GridPass(
        ViewportGridRenderer* grid,
        we::rhi::RHITextureHandle color,
        we::rhi::RHITextureHandle depth,
        we::rhi::Extent2D extent,
        const CameraUniform* camera);
    void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) override;
    void Execute(const GraphPassContext& ctx) override;

private:
    ViewportGridRenderer* m_Grid = nullptr;
    we::rhi::RHITextureHandle m_Color = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureHandle m_Depth = we::rhi::RHITextureHandle::Invalid;
    we::rhi::Extent2D m_Extent{};
    const CameraUniform* m_Camera = nullptr;
};

class RENDERER_API TonemapPass final : public RenderPass {
public:
    TonemapPass(we::rhi::RHITextureHandle hdrColor, we::rhi::RHITextureHandle swapchainImage);
    void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) override;
    void Execute(const GraphPassContext& ctx) override;

private:
    we::rhi::RHITextureHandle m_Hdr = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureHandle m_Swapchain = we::rhi::RHITextureHandle::Invalid;
};

using OverlayRecordFn = std::function<void(const GraphPassContext& ctx, we::rhi::RHITextureHandle swapImage)>;

class RENDERER_API UiOverlayPass final : public RenderPass {
public:
    UiOverlayPass(we::rhi::RHITextureHandle swapchainImage, OverlayRecordFn recorder);
    void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) override;
    void Execute(const GraphPassContext& ctx) override;

private:
    we::rhi::RHITextureHandle m_Swapchain = we::rhi::RHITextureHandle::Invalid;
    OverlayRecordFn m_Recorder;
};

class RENDERER_API PresentPass final : public RenderPass {
public:
    explicit PresentPass(we::rhi::RHITextureHandle swapchainImage);
    void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) override;
    void Execute(const GraphPassContext& ctx) override;

private:
    we::rhi::RHITextureHandle m_Swapchain = we::rhi::RHITextureHandle::Invalid;
};

// --- Named stubs (KeepAlive) -----------------------------------------------

class RENDERER_API StubGraphicsPass final : public RenderPass {
public:
    StubGraphicsPass(
        std::string name,
        uint32_t writeTextureId,
        we::rhi::ResourceState writeState = we::rhi::ResourceState::RenderTarget,
        uint32_t readTextureId = kInvalidGraphResourceId,
        we::rhi::ResourceState readState = we::rhi::ResourceState::ShaderResource);
    void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) override;
    void Execute(const GraphPassContext& ctx) override;

private:
    uint32_t m_WriteId = kInvalidGraphResourceId;
    uint32_t m_ReadId = kInvalidGraphResourceId;
    we::rhi::ResourceState m_WriteState = we::rhi::ResourceState::RenderTarget;
    we::rhi::ResourceState m_ReadState = we::rhi::ResourceState::ShaderResource;
};

// Consumes ECS ExtractedFrameData mesh/light lists (no ECS World access).
class RENDERER_API PbrOpaquePass final : public RenderPass {
public:
    PbrOpaquePass(
        uint32_t writeTextureId,
        uint32_t shadowTextureId,
        const we::runtime::ecs::ExtractedFrameData* extract);
    void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) override;
    void Execute(const GraphPassContext& ctx) override;
    [[nodiscard]] std::size_t LastMeshCount() const { return m_LastMeshCount; }

private:
    uint32_t m_WriteId = kInvalidGraphResourceId;
    uint32_t m_ShadowId = kInvalidGraphResourceId;
    const we::runtime::ecs::ExtractedFrameData* m_Extract = nullptr;
    std::size_t m_LastMeshCount = 0;
};

class RENDERER_API StubComputePass final : public RenderPass {
public:
    StubComputePass(
        std::string name,
        uint32_t writeTextureId,
        uint32_t readTextureId = kInvalidGraphResourceId);
    void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) override;
    void Execute(const GraphPassContext& ctx) override;

private:
    uint32_t m_WriteId = kInvalidGraphResourceId;
    uint32_t m_ReadId = kInvalidGraphResourceId;
};

} // namespace we::runtime::renderer

#pragma warning(pop)
