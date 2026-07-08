#pragma once

#pragma warning(disable: 4251)

#include "Renderer/Export.h"
#include "Camera/CameraSystem.h"
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

class RENDERER_API Renderer {
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


    void SetSceneViewportRect(int32_t x, int32_t y, uint32_t width, uint32_t height);

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
    bool m_Initialized = false;
    VkRect2D m_SceneViewportRect{};

};

} // namespace we::runtime::renderer
