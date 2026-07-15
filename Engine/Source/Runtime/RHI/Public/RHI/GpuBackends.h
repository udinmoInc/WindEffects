#pragma once

#include "RHI/Desc.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include "Platform/NativeHandle.h"
#include "Platform/Types.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace we::rhi {

// ---------------------------------------------------------------------------
// Backend-agnostic UI / texture bind handles for Editor + Renderer.
// ---------------------------------------------------------------------------

struct TextureBind {
    RHITextureViewHandle view = RHITextureViewHandle::Invalid;
    RHISamplerHandle sampler = RHISamplerHandle::Invalid;
    RHIDescriptorSetHandle set = RHIDescriptorSetHandle::Invalid;
};

struct UIVertex {
    float position[2]{};
    float uv[2]{};
    float color[4]{1, 1, 1, 1};
    float sdfRect[4]{};
    float sdfParams[4]{};
};

struct UIDrawBatch {
    RHIDescriptorSetHandle texture = RHIDescriptorSetHandle::Invalid;
    uint32_t indexCount = 0;
    uint32_t firstIndex = 0;
    int32_t vertexOffset = 0;
    float scissor[4]{};
    uint32_t stencilRef = 0;
    bool isText = false;
    uint32_t atlasWidth = 0;
    uint32_t atlasHeight = 0;
    float msdfPixelRange = 4.0f;
};

struct UIDrawList {
    std::vector<UIVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<UIDrawBatch> batches;
    uint32_t targetWidth = 0;
    uint32_t targetHeight = 0;
};

struct NativeGraphicsCommand {
    void* handle = nullptr;
};

struct FramePresentParams {
    RHITextureHandle colorTarget = RHITextureHandle::Invalid;
    RHITextureViewHandle targetView = RHITextureViewHandle::Invalid;
    Extent2D extent{};
    uint32_t imageIndex = 0;
    IRHICommandList* commandList = nullptr;
    NativeGraphicsCommand nativeCommand{};
};

struct SceneFrameParams {
    uint32_t frameIndex = 0;
    uint32_t imageIndex = 0;
    Extent2D viewportExtent{};
    RHITextureHandle colorTarget = RHITextureHandle::Invalid;
    RHITextureHandle depthTarget = RHITextureHandle::Invalid;
    float cameraPosition[3]{};
    float cameraView[16]{};
    float cameraProj[16]{};
    float cameraInvViewProj[16]{};
    float cameraPadding = 0.0f;
    float clearColor[4]{0.02f, 0.02f, 0.03f, 1.0f};
};

// Optional GPU scene path registered by production backends (VulkanRHI).
class RHI_API IGpuSceneBackend {
public:
    virtual ~IGpuSceneBackend() = default;
    virtual bool Initialize(IRHIDevice& device, we::platform::WindowId window,
                            const we::platform::NativeWindowHandle& nativeWindow) = 0;
    virtual void Shutdown() = 0;
    [[nodiscard]] virtual bool IsReady() const = 0;

    virtual bool BeginFrame(uint32_t& outImageIndex) = 0;
    virtual void RenderScene(const SceneFrameParams& params) = 0;
    virtual void SubmitAndPresent() = 0;
    virtual void WaitIdle() = 0;

    virtual void SetViewportColor(RHITextureHandle color) = 0;
    virtual void SetViewportDepth(RHITextureHandle depth) = 0;
    virtual void SetViewportExtent(uint32_t width, uint32_t height) = 0;
    virtual void SetViewportBlitRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h) = 0;
    [[nodiscard]] virtual RHITextureViewHandle GetViewportColorView() const = 0;
    [[nodiscard]] virtual RHISamplerHandle GetViewportColorSampler() const = 0;

    [[nodiscard]] virtual uint32_t GetSwapchainWidth() const = 0;
    [[nodiscard]] virtual uint32_t GetSwapchainHeight() const = 0;
    [[nodiscard]] virtual Format GetSwapchainFormat() const = 0;
    [[nodiscard]] virtual uint32_t GetCurrentFrameIndex() const = 0;
    [[nodiscard]] virtual uint32_t GetCurrentImageIndex() const = 0;
    [[nodiscard]] virtual IRHICommandList* GetActiveCommandList() = 0;
    [[nodiscard]] virtual void* GetNativeCommandBuffer() const = 0;
};

// Optional GPU UI path — records Overlay batches without leaking API types.
class RHI_API IGpuUIBackend {
public:
    virtual ~IGpuUIBackend() = default;
    virtual bool Initialize(IRHIDevice& device, Format swapchainFormat, uint32_t framesInFlight) = 0;
    virtual void Shutdown() = 0;

    [[nodiscard]] virtual RHIDescriptorSetHandle RegisterTexture(
        RHITextureViewHandle view, RHISamplerHandle sampler) = 0;
    virtual void UpdateTexture(
        RHIDescriptorSetHandle set, RHITextureViewHandle view, RHISamplerHandle sampler) = 0;
    virtual void UnregisterTexture(RHIDescriptorSetHandle set) = 0;
    /// Upload an RGBA8 atlas/page into a sampled UI texture and return its descriptor set.
    /// Owned by the backend until UnregisterTexture / DestroyUploadedTexture.
    [[nodiscard]] virtual RHIDescriptorSetHandle UploadRgbaTexture(
        uint32_t width, uint32_t height, std::span<const uint8_t> rgba, bool linearFilter) = 0;
    [[nodiscard]] virtual RHIDescriptorSetHandle GetDummyTexture() const = 0;
    [[nodiscard]] virtual RHISamplerHandle GetDefaultSampler() const = 0;

    virtual void BeginFrame(const FramePresentParams& target) = 0;
    virtual void SubmitDrawList(const UIDrawList& list, uint32_t frameSlot) = 0;
    virtual void EndFrame() = 0;
};

using GpuSceneBackendFactory = std::unique_ptr<IGpuSceneBackend> (*)();
using GpuUIBackendFactory = std::unique_ptr<IGpuUIBackend> (*)();

class RHI_API GpuBackendRegistry {
public:
    static void RegisterScene(GpuSceneBackendFactory factory);
    static void RegisterUI(GpuUIBackendFactory factory);
    [[nodiscard]] static std::unique_ptr<IGpuSceneBackend> CreateScene();
    [[nodiscard]] static std::unique_ptr<IGpuUIBackend> CreateUI();
    [[nodiscard]] static bool HasScene() noexcept;
    [[nodiscard]] static bool HasUI() noexcept;
};

} // namespace we::rhi
