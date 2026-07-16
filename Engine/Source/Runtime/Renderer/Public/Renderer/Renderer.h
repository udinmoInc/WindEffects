#pragma once

#pragma warning(disable : 4251)

#include "Camera/CameraUniform.h"
#include "Lighting/SceneEnvironmentUniform.h"
#include "Renderer/Export.h"
#include "Renderer/Graph/RenderGraph.h"
#include "Renderer/Graph/ScenePasses.h"
#include "Renderer/ViewportInterfaces.h"
#include "Platform/Types.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"
#include "ECS/RenderExtract.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace we::runtime::renderer {

constexpr uint32_t kMaxFramesInFlight = 2;

class ViewportSkyRenderer;
class ViewportGridRenderer;

class RENDERER_API Renderer : public ISceneViewportController {
public:
    static Renderer& Get();

    Renderer();
    ~Renderer();

    void Init(we::platform::WindowId window);
    void Shutdown();

    void RenderFrame();
    bool BeginFrame();
    void RenderScene();
    void SubmitAndPresent();

    void UploadCameraUniform(const CameraUniform& uniform);
    void UploadEnvironmentUniform(const SceneEnvironmentUniform& uniform);
    void InsertOverlayPassBarrier();

    // Bind frame-local extract packet (filled by ECS RenderExtractionSystem).
    // Renderer never owns entities — only consumes this POD packet.
    void SetExtractedFrame(const we::runtime::ecs::ExtractedFrameData* frame);
    [[nodiscard]] const we::runtime::ecs::ExtractedFrameData* GetExtractedFrame() const {
        return m_ExtractedFrame;
    }

    void SetOverlayRecorder(OverlayRecordFn recorder);
    void ClearOverlayRecorder();

    void RecordUiPresentPath(uint32_t imageIndex);
    void MarkOverlayPassEnded();

    [[nodiscard]] std::string DumpRenderGraph() const;

    void SetViewportRenderTargetSize(uint32_t width, uint32_t height) override;
    void SetViewportBlitRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
    void SetViewportRenderTargetColor(we::rhi::RHITextureHandle colorTexture) override;
    void SetViewportDepthTarget(we::rhi::RHITextureHandle depthTexture) override;
    [[nodiscard]] we::rhi::RHITextureViewHandle GetViewportColorView() const override;
    [[nodiscard]] we::rhi::RHISamplerHandle GetViewportColorSampler() const override;

    uint32_t GetViewportRenderTargetWidth() const { return m_ViewportTargetWidth; }
    uint32_t GetViewportRenderTargetHeight() const { return m_ViewportTargetHeight; }
    we::rhi::RHITextureHandle GetViewportColorTexture() const { return m_ViewportColorTexture; }
    we::rhi::RHITextureHandle GetViewportDepthTexture() const { return m_ViewportDepthTexture; }
    void GetViewportBlitRect(uint32_t& x, uint32_t& y, uint32_t& width, uint32_t& height) const;

    [[nodiscard]] we::rhi::IRHIDevice* GetRHIDevice() const { return m_RHIDevice.get(); }
    [[nodiscard]] we::rhi::IRHICommandList* GetFrameCommandList() const { return m_FrameCmd; }
    [[nodiscard]] bool IsGpuReady() const;
    [[nodiscard]] bool HasGpuScene() const { return m_RHIDevice != nullptr && m_RHIDevice->IsValid(); }

    uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
    uint32_t GetCurrentImageIndex() const { return m_CurrentImageIndex; }
    uint32_t GetSwapchainWidth() const;
    uint32_t GetSwapchainHeight() const;
    we::rhi::Format GetSwapchainFormat() const;
    void RecreateSwapchain(uint32_t width, uint32_t height);

private:
    void ResetPresentPathAudit();
    void EnsureViewportTargets();
    void DestroyViewportTargets();
    void ClearSwapchainChrome();
    void RenderViewportSky();

    std::unique_ptr<we::rhi::IRHIDevice> m_RHIDevice;
    std::unique_ptr<RenderGraph> m_RenderGraph;
    std::unique_ptr<ViewportSkyRenderer> m_ViewportSky;
    std::unique_ptr<ViewportGridRenderer> m_ViewportGrid;
    we::rhi::IRHICommandList* m_FrameCmd = nullptr;

    we::platform::WindowId m_Window = we::platform::WindowId::Invalid;
    uint32_t m_CurrentFrame = 0;
    uint32_t m_CurrentImageIndex = 0;
    bool m_FrameActive = false;
    bool m_Initialized = false;

    uint32_t m_ViewportTargetWidth = 0;
    uint32_t m_ViewportTargetHeight = 0;
    uint32_t m_ViewportBlitX = 0;
    uint32_t m_ViewportBlitY = 0;
    uint32_t m_ViewportBlitW = 0;
    uint32_t m_ViewportBlitH = 0;

    we::rhi::RHITextureHandle m_ViewportColorTexture = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureHandle m_ViewportDepthTexture = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureViewHandle m_ViewportColorView = we::rhi::RHITextureViewHandle::Invalid;
    we::rhi::RHISamplerHandle m_ViewportColorSampler = we::rhi::RHISamplerHandle::Invalid;
    uint32_t m_OwnedViewportWidth = 0;
    uint32_t m_OwnedViewportHeight = 0;

    CameraUniform m_LastCamera{};
    SceneEnvironmentUniform m_LastEnvironment{};
    const we::runtime::ecs::ExtractedFrameData* m_ExtractedFrame = nullptr;
    OverlayRecordFn m_OverlayRecorder;

    uint64_t m_PresentAuditFrameNumber = 0;
    uint32_t m_AcquiredImageIndex = UINT32_MAX;
    uint32_t m_SceneImageIndex = UINT32_MAX;
    uint32_t m_UiImageIndex = UINT32_MAX;
    bool m_OverlayPassRan = false;
    bool m_OverlayPassEnded = false;
};

} // namespace we::runtime::renderer
