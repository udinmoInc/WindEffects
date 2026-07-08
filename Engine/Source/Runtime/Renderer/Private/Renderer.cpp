#include "Renderer/Renderer.h"
#include "Core/DeviceContext.h"
#include "Core/SwapchainManager.h"
#include "Core/FrameSync.h"
#include "Resource/ResourceManager.h"
#include "Resource/DepthTarget.h"
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
#include "Core/LogCategory.h"
#include "Shader/ShaderLibrary.h"

#ifndef WE_DEBUG_UI
#define WE_DEBUG_UI 0
#endif

#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif

#include <filesystem>
#include <algorithm>
#include <cstdio>
#include <chrono>

namespace {

#ifndef WE_ENABLE_AGENT_DEBUG_LOG
#define WE_ENABLE_AGENT_DEBUG_LOG 0
#endif

#if WE_ENABLE_AGENT_DEBUG_LOG
void AgentDebugLog(const char* hypothesisId, const char* location, const char* message, const char* dataJson) {
    using namespace std::chrono;
    const auto now = time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch().count();

    std::FILE* f = std::fopen("f:/Coding/windeffects/debug-a0cfa5.log", "a");
    if (!f) return;

    std::fprintf(
        f,
        "{\"sessionId\":\"a0cfa5\",\"id\":\"log_%lld\",\"timestamp\":%lld,"
        "\"runId\":\"pre-fix\",\"hypothesisId\":\"%s\",\"location\":\"%s\","
        "\"message\":\"%s\",\"data\":%s}\n",
        static_cast<long long>(now),
        static_cast<long long>(now),
        hypothesisId,
        location,
        message,
        dataJson ? dataJson : "null");

    std::fclose(f);
}
#else
void AgentDebugLog(const char*, const char*, const char*, const char*) {}
#endif

} // namespace

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
    m_ViewportTargetExtent = {0, 0};
    m_ViewportBlitRect = {{0, 0}, {0, 0}};

    BuildRenderGraph();
    m_Initialized = true;

    // #region agent log
    {
        const VkExtent2D extent = m_SwapchainManager->GetExtent();
        const VkRect2D blitRect = m_ViewportBlitRect;
        char dataJson[256];
        std::snprintf(
            dataJson,
            sizeof(dataJson),
            "{"
            "\"swapchainExtent\":{\"w\":%u,\"h\":%u},"
            "\"viewportBlitRect\":{\"x\":%d,\"y\":%d,\"w\":%u,\"h\":%u}"
            "}",
            extent.width,
            extent.height,
            blitRect.offset.x,
            blitRect.offset.y,
            blitRect.extent.width,
            blitRect.extent.height);

        AgentDebugLog(
            "H0",
            "Renderer.cpp:Init",
            "RENDERER_INIT",
            dataJson);
    }
    // #endregion
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
        // #region agent log
        AgentDebugLog(
            "H1",
            "Renderer.cpp:AcquireNextImage",
            "FRAME_HEADER",
            "{\"acquireNextImage\":\"FAIL\"}");
        // #endregion
        return false;
    }

    m_FrameSync->ResetFence(m_CurrentFrame);
    m_FrameActive = true;

    const VkExtent2D extent = m_SwapchainManager->GetExtent();
    const VkRect2D blitRect = m_ViewportBlitRect;

    const CameraUniform& cam = m_CameraSystem->GetLastUploaded(m_CurrentFrame);

    char dataJson[512];
    std::snprintf(
        dataJson,
        sizeof(dataJson),
        "{"
        "\"frameSeparator\":\"================ FRAME =================\","
        "\"frameIndex\":%u,"
        "\"imageIndex\":%u,"
        "\"acquireNextImage\":\"PASS\","
        "\"swapchainExtent\":{\"w\":%u,\"h\":%u},"
        "\"viewportBlitRect\":{\"x\":%d,\"y\":%d,\"w\":%u,\"h\":%u},"
        "\"cameraPosition\":[%f,%f,%f]"
        "}",
        m_CurrentFrame,
        m_CurrentImageIndex,
        extent.width,
        extent.height,
        blitRect.offset.x,
        blitRect.offset.y,
        blitRect.extent.width,
        blitRect.extent.height,
#if WE_HAS_GLM
        cam.position.x,
        cam.position.y,
        cam.position.z
#else
        cam.position[0],
        cam.position[1],
        cam.position[2]
#endif
    );

    // #region agent log
    AgentDebugLog(
        "H1",
        "Renderer.cpp:BeginFrame",
        "FRAME_HEADER",
        dataJson);
    // #endregion

    return true;
}

void Renderer::TransitionFrameImages(VkCommandBuffer cmd) {
    if (cmd == VK_NULL_HANDLE) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(), "TransitionFrameImages called with null command buffer.");
        return;
    }
    const auto& swapchainImages = m_SwapchainManager->GetImages();
    if (m_CurrentImageIndex >= swapchainImages.size()) {
        WE_LOG_ERROR(
            we::LogCategory::Renderer.data(),
            "TransitionFrameImages skipped: swapchain image index out of range.");
        return;
    }

    const VkImageLayout oldColorLayout = m_HasPresentedSwapchainImage
        ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        : VK_IMAGE_LAYOUT_UNDEFINED;

    // Swapchain is written by the editor compositor + UI overlay.
    TransitionImageLayout(
        cmd,
        swapchainImages[m_CurrentImageIndex],
        oldColorLayout,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        oldColorLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_ACCESS_MEMORY_READ_BIT : 0,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        oldColorLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    // Viewport scene renders into an offscreen texture which is later sampled by the editor compositor.
    if (m_ViewportColorImage != VK_NULL_HANDLE) {
        const VkImageLayout oldViewportLayout = m_ViewportColorInShaderRead
            ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            : VK_IMAGE_LAYOUT_UNDEFINED;

        TransitionImageLayout(
            cmd,
            m_ViewportColorImage,
            oldViewportLayout,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            m_ViewportColorInShaderRead ? VK_ACCESS_SHADER_READ_BIT : 0,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            m_ViewportColorInShaderRead ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        m_ViewportColorInShaderRead = false;
    }

    if (m_ViewportDepthTarget != nullptr && m_ViewportDepthTarget->GetImage() != VK_NULL_HANDLE) {
        if (!m_DepthImageReady) {
            TransitionDepthImageLayout(
                cmd,
                m_ViewportDepthTarget->GetImage(),
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
    frame.extent = m_ViewportTargetExtent;
    frame.sceneRenderArea = {{0, 0}, m_ViewportTargetExtent};
    frame.colorImage = m_ViewportColorImage;
    frame.colorImageView = m_ViewportColorImageView;
    frame.colorFormat = m_ViewportColorFormat;
    frame.camera = m_CameraSystem.get();
    frame.directionalLight = m_SceneRenderer->GetDirectionalLight();
    frame.depthTarget = m_ViewportDepthTarget;
    frame.sceneRenderer = m_SceneRenderer.get();

    m_RenderGraph->Execute(frame);
}

void Renderer::ClearSwapchainBackground(VkCommandBuffer cmd) {
    const auto& swapchainImages = m_SwapchainManager->GetImages();
    if (cmd == VK_NULL_HANDLE || m_CurrentImageIndex >= swapchainImages.size()) {
        return;
    }

    VkImage swapImage = swapchainImages[m_CurrentImageIndex];

    TransitionImageLayout(
        cmd,
        swapImage,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkClearColorValue clearColor{};
    clearColor.float32[0] = 0.09f;
    clearColor.float32[1] = 0.09f;
    clearColor.float32[2] = 0.09f;
    clearColor.float32[3] = 1.0f;

    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.levelCount = 1;
    range.layerCount = 1;

    vkCmdClearColorImage(cmd, swapImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);

    TransitionImageLayout(
        cmd,
        swapImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

void Renderer::RenderScene() {
    WE_VALIDATE_RENDER(m_Initialized, "Renderer::RenderScene", "Renderer not initialized.");
    WE_VALIDATE_RENDER(m_FrameActive, "Renderer::RenderScene", "No active frame.");

    VkCommandBuffer cmd = m_CommandContext->BeginFrame(m_CurrentFrame);
    if (cmd == VK_NULL_HANDLE) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(), "RenderScene aborted: BeginFrame returned null command buffer.");
        return;
    }
    TransitionFrameImages(cmd);
    ClearSwapchainBackground(cmd);

    if (m_ViewportColorImageView == VK_NULL_HANDLE ||
        m_ViewportDepthTarget == nullptr ||
        m_ViewportDepthTarget->GetImageView() == VK_NULL_HANDLE ||
        m_ViewportTargetExtent.width == 0 ||
        m_ViewportTargetExtent.height == 0 ||
        m_ViewportBlitRect.extent.width == 0 ||
        m_ViewportBlitRect.extent.height == 0) {
        return;
    }
    if (!m_RenderGraph || !m_SceneRenderer || !m_SceneRenderer->GetDirectionalLight()) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(), "RenderScene skipped: renderer graph dependencies not ready.");
        return;
    }

    ExecuteFoundationPasses(cmd);

    // Copy viewport offscreen color target into the swapchain at viewport rect.
    const auto& swapchainImages = m_SwapchainManager->GetImages();
    if (m_ViewportColorImage != VK_NULL_HANDLE && m_CurrentImageIndex < swapchainImages.size()) {
        if ((m_CurrentFrame % 60u) == 0u) {
            WE_LOG_INFO(
                we::LogCategory::Renderer.data(),
                std::string("[RendererScene] copy viewport->swapchain frame=") + std::to_string(m_CurrentFrame) +
                " image=" + std::to_string(m_CurrentImageIndex) +
                " viewportImage=0x" + std::to_string(reinterpret_cast<uint64_t>(m_ViewportColorImage)) +
                " viewportView=0x" + std::to_string(reinterpret_cast<uint64_t>(m_ViewportColorImageView)) +
                " swapImage=0x" + std::to_string(reinterpret_cast<uint64_t>(swapchainImages[m_CurrentImageIndex])) +
                " blitOffset=" + std::to_string(m_ViewportBlitRect.offset.x) + "," + std::to_string(m_ViewportBlitRect.offset.y) +
                " blitExtent=" + std::to_string(m_ViewportBlitRect.extent.width) + "x" + std::to_string(m_ViewportBlitRect.extent.height) +
                " targetExtent=" + std::to_string(m_ViewportTargetExtent.width) + "x" + std::to_string(m_ViewportTargetExtent.height));
        }

        TransitionImageLayout(
            cmd,
            m_ViewportColorImage,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);

        TransitionImageLayout(
            cmd,
            swapchainImages[m_CurrentImageIndex],
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkImageCopy region{};
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount = 1;
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.baseArrayLayer = 0;
        region.dstSubresource.layerCount = 1;
        region.extent = {
            m_ViewportBlitRect.extent.width,
            m_ViewportBlitRect.extent.height,
            1};
        region.dstOffset = {
            m_ViewportBlitRect.offset.x,
            m_ViewportBlitRect.offset.y,
            0};

        vkCmdCopyImage(
            cmd,
            m_ViewportColorImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            swapchainImages[m_CurrentImageIndex],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

        TransitionImageLayout(
            cmd,
            swapchainImages[m_CurrentImageIndex],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        TransitionImageLayout(
            cmd,
            m_ViewportColorImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        m_ViewportColorInShaderRead = true;
    }
}

void Renderer::SubmitAndPresent() {
    VkCommandBuffer cmd = m_CommandContext->GetCurrentCommandBuffer(m_CurrentFrame);
    if (cmd == VK_NULL_HANDLE) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(), "SubmitAndPresent aborted: current command buffer is null.");
        m_FrameActive = false;
        m_CurrentFrame = (m_CurrentFrame + 1) % kMaxFramesInFlight;
        return;
    }
    const auto& swapchainImages = m_SwapchainManager->GetImages();
    if (m_CurrentImageIndex >= swapchainImages.size()) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(), "SubmitAndPresent aborted: swapchain image index out of range.");
        m_FrameActive = false;
        m_CurrentFrame = (m_CurrentFrame + 1) % kMaxFramesInFlight;
        return;
    }

    TransitionImageLayout(
        cmd,
        swapchainImages[m_CurrentImageIndex],
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

    m_CommandContext->EndFrame(m_CurrentFrame);
    
    #if WE_DEBUG_UI
    WE_LOG_INFO(we::LogCategory::Renderer.data(),
        std::string("QueueSubmit | Swapchain Image Index: ") + std::to_string(m_CurrentImageIndex));
    #endif
    m_SwapchainManager->SubmitFrame(m_CurrentFrame, m_CurrentImageIndex, cmd);
    
    #if WE_DEBUG_UI
    WE_LOG_INFO(we::LogCategory::Renderer.data(),
        std::string("Present | Swapchain Image Index: ") + std::to_string(m_CurrentImageIndex));
    #endif
    m_SwapchainManager->Present(m_CurrentFrame, m_CurrentImageIndex);
    m_HasPresentedSwapchainImage = true;

    m_FrameActive = false;
    m_CurrentFrame = (m_CurrentFrame + 1) % kMaxFramesInFlight;
}

void Renderer::RenderFrame() {
    if (!BeginFrame()) {
        return;
    }
    RenderScene();
    SubmitAndPresent();
}

void Renderer::UploadCameraUniform(const CameraUniform& uniform) {
    WE_VALIDATE_RENDER(m_Initialized, "Renderer::UploadCameraUniform", "Renderer not initialized.");

    CameraUniform upload = uniform;
#if WE_HAS_GLM
    upload.invViewProj = glm::inverse(upload.proj * upload.view);
#endif
    m_CameraSystem->Upload(m_CurrentFrame, upload);
}

void Renderer::InsertOverlayPassBarrier() {
    if (!m_FrameActive) {
        return;
    }

    VkCommandBuffer cmd = m_CommandContext->GetCurrentCommandBuffer(m_CurrentFrame);
    if (cmd == VK_NULL_HANDLE) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(), "InsertOverlayPassBarrier skipped: current command buffer is null.");
        return;
    }

    VkMemoryBarrier memoryBarrier{};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        1,
        &memoryBarrier,
        0,
        nullptr,
        0,
        nullptr);
}

void Renderer::SetViewportRenderTargetSize(uint32_t width, uint32_t height) {
    m_ViewportTargetExtent = {width, height};
    m_DepthImageReady = false;
    WE_LOG_INFO(
        we::LogCategory::Renderer.data(),
        std::string("[RendererScene] SetViewportRenderTargetSize ") +
        std::to_string(width) + "x" + std::to_string(height));
}

void Renderer::SetViewportBlitRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    const VkRect2D next{
        {static_cast<int32_t>(x), static_cast<int32_t>(y)},
        {width, height}};
    if (m_ViewportBlitRect.offset.x == next.offset.x &&
        m_ViewportBlitRect.offset.y == next.offset.y &&
        m_ViewportBlitRect.extent.width == next.extent.width &&
        m_ViewportBlitRect.extent.height == next.extent.height) {
        return;
    }

    m_ViewportBlitRect = next;
    WE_LOG_INFO(
        we::LogCategory::Renderer.data(),
        std::string("[RendererScene] SetViewportBlitRect ") +
        std::to_string(x) + "," + std::to_string(y) + " " +
        std::to_string(width) + "x" + std::to_string(height));
}

void Renderer::SetViewportRenderTargetColor(VkImage colorImage, VkImageView colorImageView, VkFormat colorFormat) {
    m_ViewportColorImage = colorImage;
    m_ViewportColorImageView = colorImageView;
    m_ViewportColorFormat = colorFormat;
    m_ViewportColorInShaderRead = false;
    WE_LOG_INFO(
        we::LogCategory::Renderer.data(),
        std::string("[RendererScene] SetViewportRenderTargetColor image=0x") +
        std::to_string(reinterpret_cast<uint64_t>(colorImage)) +
        " view=0x" + std::to_string(reinterpret_cast<uint64_t>(colorImageView)) +
        " format=" + std::to_string(static_cast<uint32_t>(colorFormat)));
}

void Renderer::SetViewportDepthTarget(DepthTarget* depthTarget) {
    m_ViewportDepthTarget = depthTarget;
    m_DepthImageReady = false;
    VkImage depthImage = VK_NULL_HANDLE;
    VkImageView depthView = VK_NULL_HANDLE;
    if (depthTarget != nullptr) {
        depthImage = depthTarget->GetImage();
        depthView = depthTarget->GetImageView();
    }
    WE_LOG_INFO(
        we::LogCategory::Renderer.data(),
        std::string("[RendererScene] SetViewportDepthTarget ptr=0x") +
        std::to_string(reinterpret_cast<uint64_t>(depthTarget)) +
        " image=0x" + std::to_string(reinterpret_cast<uint64_t>(depthImage)) +
        " view=0x" + std::to_string(reinterpret_cast<uint64_t>(depthView)));
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
    if (m_ViewportTargetExtent.width != 0 && m_ViewportTargetExtent.height != 0) {
        SetViewportRenderTargetSize(m_ViewportTargetExtent.width, m_ViewportTargetExtent.height);
    }
    m_DepthImageReady = false;
}

} // namespace we::runtime::renderer
