#pragma once

#pragma warning(disable: 4251)

#include "Renderer/Export.h"
#include "Camera/CameraSystem.h"
#include "Resource/DepthTarget.h"
#include "Renderer/ViewportInterfaces.h"
#include <functional>
#include <memory>
#include <volk.h>

struct SDL_Window;

namespace we::runtime::renderer {

constexpr uint32_t kMaxFramesInFlight = 2;
class DeviceContext;
class SwapchainManager;
class ResourceManager;
class CommandContext;
class FrameSync;
class CameraSystem;
class RenderGraph;
class SceneRenderer;
class GraphicsPipelineFactory;

class RENDERER_API Renderer : public ISceneViewportController {
public:
    static Renderer& Get();

    Renderer();
    ~Renderer();

    void Init(SDL_Window* window);
    void Shutdown();

    void RenderFrame();
    bool BeginFrame();
    void RenderScene();
    void SubmitAndPresent();

    void UploadCameraUniform(const CameraUniform& uniform);
    void InsertOverlayPassBarrier();

    // Editor-facing viewport control (implements ISceneViewportController)
    void SetViewportRenderTargetSize(uint32_t width, uint32_t height) override;
    void SetViewportBlitRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
    void SetViewportRenderTargetColor(VkImage colorImage, VkImageView colorImageView, VkFormat colorFormat) override;
    void SetViewportDepthTarget(DepthTarget* depthTarget) override;

    // Offscreen viewport render target accessors (used by editor compositor)
    uint32_t GetViewportRenderTargetWidth() const { return m_ViewportTargetExtent.width; }
    uint32_t GetViewportRenderTargetHeight() const { return m_ViewportTargetExtent.height; }
    VkImage GetViewportColorImage() const { return m_ViewportColorImage; }
    VkImageView GetViewportColorImageView() const { return m_ViewportColorImageView; }
    VkFormat GetViewportColorFormat() const { return m_ViewportColorFormat; }

    DeviceContext* GetDeviceContext() const { return m_DeviceContext.get(); }
    SwapchainManager* GetSwapchainManager() const { return m_SwapchainManager.get(); }
    ResourceManager* GetResourceManager() const { return m_ResourceManager.get(); }
    CommandContext* GetCommandContext() const { return m_CommandContext.get(); }
    FrameSync* GetFrameSync() const { return m_FrameSync.get(); }
    CameraSystem* GetCameraSystem() const { return m_CameraSystem.get(); }
    RenderGraph* GetRenderGraph() const { return m_RenderGraph.get(); }
    SceneRenderer* GetSceneRenderer() const { return m_SceneRenderer.get(); }

    VkCommandBuffer GetCommandBuffer() const;
    VkBuffer GetCameraBuffer() const;
    VkDescriptorSetLayout GetCameraDescLayout() const;
    VkDescriptorSet GetCameraDescSet() const;
    uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
    uint32_t GetCurrentImageIndex() const { return m_CurrentImageIndex; }
    uint32_t GetSwapchainWidth() const;
    uint32_t GetSwapchainHeight() const;
    VkFormat GetSwapchainImageFormat() const;
    void RecreateSwapchain(uint32_t width, uint32_t height);

private:
    void InitializeShaderLibrary();
    void BuildRenderGraph();
    void ExecuteFoundationPasses(VkCommandBuffer cmd);
    void TransitionFrameImages(VkCommandBuffer cmd);
    void ClearSwapchainBackground(VkCommandBuffer cmd);

    std::unique_ptr<DeviceContext> m_DeviceContext;
    std::unique_ptr<SwapchainManager> m_SwapchainManager;
    std::unique_ptr<ResourceManager> m_ResourceManager;
    std::unique_ptr<CommandContext> m_CommandContext;
    std::unique_ptr<FrameSync> m_FrameSync;
    std::unique_ptr<CameraSystem> m_CameraSystem;
    std::unique_ptr<GraphicsPipelineFactory> m_PipelineFactory;
    std::unique_ptr<RenderGraph> m_RenderGraph;
    std::unique_ptr<SceneRenderer> m_SceneRenderer;

    SDL_Window* m_Window = nullptr;
    uint32_t m_CurrentFrame = 0;
    uint32_t m_CurrentImageIndex = 0;
    bool m_FrameActive = false;
    bool m_HasPresentedSwapchainImage = false;
    bool m_DepthImageReady = false;
    bool m_ViewportColorInShaderRead = false;
    bool m_Initialized = false;
    VkExtent2D m_ViewportTargetExtent{};
    VkRect2D m_ViewportBlitRect{};

    // Offscreen viewport render target (color).
    VkImage m_ViewportColorImage = VK_NULL_HANDLE;
    VkImageView m_ViewportColorImageView = VK_NULL_HANDLE;
    VkFormat m_ViewportColorFormat = VK_FORMAT_UNDEFINED;
    DepthTarget* m_ViewportDepthTarget = nullptr;

};

} // namespace we::runtime::renderer
