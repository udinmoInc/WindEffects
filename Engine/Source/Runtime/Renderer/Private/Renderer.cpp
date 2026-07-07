#include "Renderer/Renderer.h"
#include "Core/DeviceContext.h"
#include "Core/SwapchainManager.h"
#include "Core/FrameSync.h"
#include "Resource/ResourceManager.h"
#include "Core/CommandContext.h"
#include "Core/ImageBarriers.h"
#include "Graph/RenderGraph.h"
#include "Graph/FrameContext.h"
#include "Scene/SceneRenderer.h"
#include "Camera/CameraSystem.h"
#include "Pipeline/GraphicsPipelineFactory.h"
#include "Passes/SkyPass.h"
#include "Passes/PBRPass.h"
#include "Core/Validation.h"
#include "Shader/ShaderLibrary.h"

#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif

#include <filesystem>

namespace we::runtime::renderer {

static Renderer* s_Instance = nullptr;

Renderer& Renderer::Get() {
    WE_VALIDATE_INIT(s_Instance != nullptr, "Renderer", "Renderer instance is null. Was it created?");
    return *s_Instance;
}

Renderer::Renderer() {
    WE_VALIDATE_INIT(s_Instance == nullptr, "Renderer", "Renderer instance already exists!");
    s_Instance = this;

    m_DeviceContext = std::make_unique<DeviceContext>();
    m_SwapchainManager = std::make_unique<SwapchainManager>();
    m_ResourceManager = std::make_unique<ResourceManager>();
    m_CommandContext = std::make_unique<CommandContext>();
    m_FrameSync = std::make_unique<FrameSync>();
    m_CameraSystem = std::make_unique<CameraSystem>();
    m_RenderGraph = std::make_unique<RenderGraph>();
    m_SceneRenderer = std::make_unique<SceneRenderer>();
}

Renderer::~Renderer() {
    Shutdown();
    s_Instance = nullptr;
}

void Renderer::InitializeShaderLibrary() {
    std::string shaderRoot = "Engine/Shaders";
    std::string bytecodeRoot = "Engine/Shaders/Bytecodes";

    for (const char* candidate : {"Engine/Shaders", "../Engine/Shaders", "../../Engine/Shaders"}) {
        if (std::filesystem::exists(candidate)) {
            shaderRoot = candidate;
            break;
        }
    }

    for (const char* candidate : {
             "Engine/Shaders/Bytecodes",
             "../Engine/Shaders/Bytecodes",
             "../../Engine/Shaders/Bytecodes",
             "Assets/Shaders"}) {
        if (std::filesystem::exists(candidate)) {
            bytecodeRoot = candidate;
            break;
        }
    }

    ShaderLibrary::Get().Initialize(shaderRoot, bytecodeRoot);
}

void Renderer::Init(SDL_Window* window) {
    WE_VALIDATE_INIT(!m_Initialized, "Renderer", "Renderer already initialized.");
    WE_VALIDATE_INIT(window != nullptr, "Renderer", "Window is null.");

    m_Window = window;
    InitializeShaderLibrary();

    DeviceContextConfig deviceConfig{};
    deviceConfig.window = window;
    m_DeviceContext->Init(deviceConfig);
    m_PipelineFactory = std::make_unique<GraphicsPipelineFactory>(m_DeviceContext.get());

    SwapchainManagerConfig swapConfig{};
    swapConfig.deviceContext = m_DeviceContext.get();
    swapConfig.window = window;
    m_SwapchainManager->Init(swapConfig);

    ResourceManagerConfig resConfig{};
    resConfig.deviceContext = m_DeviceContext.get();
    m_ResourceManager->Init(resConfig);

    CommandContextConfig cmdConfig{};
    cmdConfig.deviceContext = m_DeviceContext.get();
    cmdConfig.maxFramesInFlight = kMaxFramesInFlight;
    m_CommandContext->Init(cmdConfig);

    FrameSyncConfig syncConfig{};
    syncConfig.deviceContext = m_DeviceContext.get();
    syncConfig.maxFramesInFlight = kMaxFramesInFlight;
    m_FrameSync->Init(syncConfig);
    m_SwapchainManager->SetFrameSync(m_FrameSync.get());

    CameraSystemConfig cameraConfig{};
    cameraConfig.deviceContext = m_DeviceContext.get();
    cameraConfig.resourceManager = m_ResourceManager.get();
    cameraConfig.maxFramesInFlight = kMaxFramesInFlight;
    m_CameraSystem->Init(cameraConfig);

    SceneRendererConfig sceneConfig{};
    sceneConfig.renderer = this;
    sceneConfig.deviceContext = m_DeviceContext.get();
    sceneConfig.resourceManager = m_ResourceManager.get();
    sceneConfig.pipelineFactory = m_PipelineFactory.get();
    sceneConfig.maxFramesInFlight = kMaxFramesInFlight;
    sceneConfig.colorFormat = m_SwapchainManager->GetImageFormat();
    sceneConfig.depthFormat = VK_FORMAT_D32_SFLOAT;
    m_SceneRenderer->Init(sceneConfig);
    m_SceneRenderer->Resize(m_SwapchainManager->GetExtent().width, m_SwapchainManager->GetExtent().height);

    BuildRenderGraph();
    m_Initialized = true;
}

void Renderer::BuildRenderGraph() {
    auto skyPass = std::make_unique<SkyPass>();
    SkyPassConfig skyConfig{};
    skyConfig.deviceContext = m_DeviceContext.get();
    skyConfig.pipelineFactory = m_PipelineFactory.get();
    skyConfig.cameraDescriptorSetLayout = m_CameraSystem->GetDescriptorSetLayout();
    skyConfig.colorFormat = m_SwapchainManager->GetImageFormat();
    skyPass->Init(skyConfig);

    auto pbrPass = std::make_unique<PBRPass>();
    PBRPassConfig pbrConfig{};
    pbrConfig.deviceContext = m_DeviceContext.get();
    pbrConfig.pipelineFactory = m_PipelineFactory.get();
    pbrConfig.cameraDescriptorSetLayout = m_CameraSystem->GetDescriptorSetLayout();
    pbrConfig.objectDescriptorSetLayout = m_SceneRenderer->GetObjectDescriptorSetLayout();
    pbrConfig.lightDescriptorSetLayout = m_SceneRenderer->GetDirectionalLight()->GetDescriptorSetLayout();
    pbrConfig.colorFormat = m_SwapchainManager->GetImageFormat();
    pbrConfig.depthFormat = VK_FORMAT_D32_SFLOAT;
    pbrPass->Init(pbrConfig);

    m_RenderGraph->Init(this);
    m_RenderGraph->AddPass(std::move(skyPass));
    m_RenderGraph->AddPass(std::move(pbrPass));
    m_RenderGraph->Compile();
}

void Renderer::Shutdown() {
    if (!m_Initialized) return;

    if (m_DeviceContext->GetDevice() != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_DeviceContext->GetDevice());
    }

    m_RenderGraph->Shutdown();
    m_SceneRenderer->Shutdown();
    m_CameraSystem->Shutdown();
    m_FrameSync->Shutdown();
    m_CommandContext->Shutdown();
    m_ResourceManager->Shutdown();
    m_SwapchainManager->Shutdown();
    m_DeviceContext->Shutdown();

    m_Initialized = false;
    m_FrameActive = false;
}

bool Renderer::BeginFrame() {
    WE_VALIDATE_RENDER(m_Initialized, "Renderer::BeginFrame", "Renderer not initialized.");
    WE_VALIDATE_RENDER(!m_FrameActive, "Renderer::BeginFrame", "Frame already active.");

    m_FrameSync->WaitForFrame(m_CurrentFrame);

    if (!m_SwapchainManager->AcquireNextImage(m_CurrentFrame, m_CurrentImageIndex)) {
        return false;
    }

    m_FrameSync->ResetFence(m_CurrentFrame);
    if (m_SceneRenderer->ResizeIfNeeded(m_SwapchainManager->GetExtent().width, m_SwapchainManager->GetExtent().height)) {
        m_DepthImageReady = false;
    }
    m_FrameActive = true;
    return true;
}

void Renderer::TransitionFrameImages(VkCommandBuffer cmd) {
    const VkImageLayout oldColorLayout = m_HasPresentedSwapchainImage
        ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        : VK_IMAGE_LAYOUT_UNDEFINED;

    TransitionImageLayout(
        cmd,
        m_SwapchainManager->GetImages()[m_CurrentImageIndex],
        oldColorLayout,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        oldColorLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_ACCESS_MEMORY_READ_BIT : 0,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        oldColorLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    if (m_SceneRenderer->GetDepthTarget()->GetImage() != VK_NULL_HANDLE) {
        if (!m_DepthImageReady) {
            TransitionDepthImageLayout(
                cmd,
                m_SceneRenderer->GetDepthTarget()->GetImage(),
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            m_DepthImageReady = true;
        }
    }
}

void Renderer::ExecuteFoundationPasses(VkCommandBuffer cmd) {
    FrameContext frame{};
    frame.frameIndex = m_CurrentFrame;
    frame.imageIndex = m_CurrentImageIndex;
    frame.commandBuffer = cmd;
    frame.extent = m_SwapchainManager->GetExtent();
    frame.colorImage = m_SwapchainManager->GetImages()[m_CurrentImageIndex];
    frame.colorImageView = m_SwapchainManager->GetImageViews()[m_CurrentImageIndex];
    frame.colorFormat = m_SwapchainManager->GetImageFormat();
    frame.camera = m_CameraSystem.get();
    frame.directionalLight = m_SceneRenderer->GetDirectionalLight();
    frame.depthTarget = m_SceneRenderer->GetDepthTarget();
    frame.sceneRenderer = m_SceneRenderer.get();

    m_RenderGraph->Execute(frame);
}

void Renderer::EndFrame() {
    WE_VALIDATE_RENDER(m_Initialized, "Renderer::EndFrame", "Renderer not initialized.");
    WE_VALIDATE_RENDER(m_FrameActive, "Renderer::EndFrame", "No active frame.");

    VkCommandBuffer cmd = m_CommandContext->BeginFrame(m_CurrentFrame);
    TransitionFrameImages(cmd);
    ExecuteFoundationPasses(cmd);

    // Ensure foundation passes complete before UI overlay begins
    VkMemoryBarrier memoryBarrier{};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        1, &memoryBarrier,
        0, nullptr,
        0, nullptr);

    if (m_UiOverlayRecorder) {
        const VkExtent2D extent = m_SwapchainManager->GetExtent();
        m_UiOverlayRecorder(
            cmd,
            m_SwapchainManager->GetImageViews()[m_CurrentImageIndex],
            m_SwapchainManager->GetImageFormat(),
            extent.width,
            extent.height,
            m_CurrentFrame);
    }

    TransitionImageLayout(
        cmd,
        m_SwapchainManager->GetImages()[m_CurrentImageIndex],
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

    m_CommandContext->EndFrame(m_CurrentFrame);
    m_SwapchainManager->SubmitFrame(m_CurrentFrame, m_CurrentImageIndex, cmd);
    m_SwapchainManager->Present(m_CurrentFrame, m_CurrentImageIndex);
    m_HasPresentedSwapchainImage = true;

    m_FrameActive = false;
    m_CurrentFrame = (m_CurrentFrame + 1) % kMaxFramesInFlight;
}

void Renderer::RenderFrame() {
    if (!BeginFrame()) {
        return;
    }
    EndFrame();
}

void Renderer::UploadCameraUniform(const CameraUniform& uniform) {
    WE_VALIDATE_RENDER(m_Initialized, "Renderer::UploadCameraUniform", "Renderer not initialized.");

    CameraUniform upload = uniform;
#if WE_HAS_GLM
    upload.invViewProj = glm::inverse(upload.proj * upload.view);
#endif
    m_CameraSystem->Upload(m_CurrentFrame, upload);
}

void Renderer::SetUiOverlayRecorder(UiOverlayRecorder recorder) {
    m_UiOverlayRecorder = std::move(recorder);
}

VkCommandBuffer Renderer::GetCommandBuffer() const {
    return m_CommandContext->GetCurrentCommandBuffer(m_CurrentFrame);
}

VkBuffer Renderer::GetCameraBuffer() const {
    return m_CameraSystem->GetBuffer(m_CurrentFrame);
}

VkDescriptorSetLayout Renderer::GetCameraDescLayout() const {
    return m_CameraSystem->GetDescriptorSetLayout();
}

VkDescriptorSet Renderer::GetCameraDescSet() const {
    return m_CameraSystem->GetDescriptorSet(m_CurrentFrame);
}

uint32_t Renderer::GetSwapchainWidth() const {
    return m_SwapchainManager->GetExtent().width;
}

uint32_t Renderer::GetSwapchainHeight() const {
    return m_SwapchainManager->GetExtent().height;
}

VkFormat Renderer::GetSwapchainImageFormat() const {
    return m_SwapchainManager->GetImageFormat();
}

void Renderer::RecreateSwapchain(uint32_t /*width*/, uint32_t /*height*/) {
    if (!m_Initialized) {
        return;
    }
    m_SwapchainManager->Recreate();
    if (m_SceneRenderer) {
        m_SceneRenderer->Resize(m_SwapchainManager->GetExtent().width, m_SwapchainManager->GetExtent().height);
    }
    m_DepthImageReady = false;
}

} // namespace we::runtime::renderer
