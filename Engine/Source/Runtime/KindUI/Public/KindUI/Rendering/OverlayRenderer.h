// ==============================================================================
// WindEffects — KindUI — OverlayRenderer
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Profiling/UiBuildPhaseTiming.h"
#include "KindUI/Rendering/OverlayRenderContext.h"
#include "RHI/GpuBackends.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

namespace we::runtime::kindui {
class UiImmediateRenderer;
}

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

namespace we::runtime::kindui {

class Widget;
class UIWidgetAdapter;
class UIStateManager;
class IconRenderer;
class IconManager;
class TextUIService;
class UiGpuUpload;

struct KINDUI_API UIVertex2 {
    float position[2];
    float uv[2];
    float color[4];
    float sdfRect[4];
    float sdfParams[4];
};

struct UIRenderBatch {
    we::rhi::RHIDescriptorSetHandle textureSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    uint32_t indexCount = 0;
    uint32_t firstIndex = 0;
    int32_t vertexOffset = 0;
    float scissor[4]{};
    uint32_t stencilRef = 0;
    bool isText = false;
    bool opaqueReplace = false;
    uint32_t atlasWidth = 0;
    uint32_t atlasHeight = 0;
    float msdfPixelRange = 4.0f;
};

struct UIFrameStats {
    uint32_t drawCalls = 0;
    uint32_t vertices = 0;
    uint32_t indices = 0;
    uint32_t batches = 0;
    uint32_t opaqueBatches = 0;
    uint32_t alphaBatches = 0;
    uint32_t opaqueIndices = 0;
    uint32_t alphaIndices = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct UIDirtyRegion {
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

class KINDUI_API OverlayRenderer {
public:
    OverlayRenderer();
    ~OverlayRenderer();

    bool Init(we::rhi::IRHIDevice* device, we::rhi::Format swapchainFormat, uint32_t maxFramesInFlight);
    void Shutdown();

    void BeginOverlayPass(const we::runtime::uigfx::OverlayRenderContext& context);
    void SetTargetExtent(uint32_t width, uint32_t height);
    void RenderUI(const std::shared_ptr<we::runtime::kindui::Widget>& root, uint32_t frameSlot);
    void EndOverlayPass(const we::runtime::uigfx::OverlayRenderContext& context);
    void SetPipelineAuditImageIndex(uint32_t imageIndex);

    we::rhi::RHIDescriptorSetHandle RegisterTexture(
        we::rhi::RHITextureViewHandle imageView,
        we::rhi::RHISamplerHandle sampler);
    void UpdateTexture(
        we::rhi::RHIDescriptorSetHandle descriptorSet,
        we::rhi::RHITextureViewHandle imageView,
        we::rhi::RHISamplerHandle sampler);
    void UnregisterTexture(we::rhi::RHIDescriptorSetHandle descriptorSet);
    [[nodiscard]] we::rhi::RHIDescriptorSetHandle UploadRgbaTexture(
        uint32_t width,
        uint32_t height,
        std::span<const uint8_t> rgba,
        bool linearFilter = false,
        bool srgb = false);

    TextUIService* GetTextUIService() const;
    IconRenderer* GetIconRenderer() const;
    IconManager* GetIconManager() const;
    UiGpuUpload* GetGpuUpload() const { return m_GpuUpload.get(); }
    we::rhi::RHIDescriptorSetHandle GetDummyDescriptorSet() const {
        return static_cast<we::rhi::RHIDescriptorSetHandle>(m_DummyDescriptorSet);
    }
    we::rhi::RHISamplerHandle GetDefaultSampler() const {
        return static_cast<we::rhi::RHISamplerHandle>(m_DummySampler);
    }

    const UIFrameStats& GetFrameStats() const { return m_FrameStats; }
    [[nodiscard]] uint64_t GetGeometryGeneration() const { return m_GeometryGeneration; }

    /// CPU ms spent in RenderUI (widget walk + geometry build). 0 when idle-skipped.
    [[nodiscard]] float GetLastBuildCpuMs() const { return m_LastBuildCpuMs; }
    [[nodiscard]] const UiBuildPhaseTiming& GetLastBuildPhases() const { return m_LastPhaseTiming; }
    /// CPU ms spent recording UI draw commands in EndOverlayPass.
    [[nodiscard]] float GetLastSubmitCpuMs() const { return m_LastSubmitCpuMs; }
    [[nodiscard]] bool BuiltGeometryThisFrame() const { return m_BuiltGeometryThisFrame; }
    [[nodiscard]] bool SubmittedGpuThisFrame() const { return m_SubmittedGpuThisFrame; }
    [[nodiscard]] bool UploadedGeometryThisFrame() const { return m_UploadedGeometryThisFrame; }
    [[nodiscard]] bool LastSubmissionCacheHit() const { return m_LastSubmissionCacheHit; }
    [[nodiscard]] bool LastSubmissionRebuilt() const { return m_LastSubmissionRebuilt; }
    [[nodiscard]] uint64_t SubmissionCacheHitCount() const;
    [[nodiscard]] uint64_t SubmissionCacheMissCount() const;
    [[nodiscard]] uint64_t SubmissionRebuildCount() const;
    [[nodiscard]] uint64_t SubmissionInvalidationCount() const;

    /// Hard-invalidate reusable GPU UI submissions (swapchain recreate, device loss).
    void InvalidateGpuSubmissionCache();
    void SetSwapchainFormat(we::rhi::Format format);

private:
#pragma warning(push)
#pragma warning(disable : 4251)
    std::unique_ptr<UIWidgetAdapter> m_WidgetAdapter;
    std::unique_ptr<UIStateManager> m_StateManager;
    std::unique_ptr<UiGpuUpload> m_GpuUpload;
    std::unique_ptr<UiImmediateRenderer> m_UIImmediate;

    we::rhi::IRHIDevice* m_RHIDevice = nullptr;
    we::rhi::Format m_SwapchainFormat = we::rhi::Format::Unknown;
    uint32_t m_MaxFramesInFlight = 2;

    uint64_t m_DummySampler = 0;
    uint64_t m_DummyDescriptorSet = 0;

    std::unique_ptr<TextUIService> m_TextUIService;
    std::unique_ptr<IconManager> m_IconManager;
    std::unique_ptr<IconRenderer> m_IconRenderer;

    std::vector<UIVertex2> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::vector<UIRenderBatch> m_Batches;
    we::rhi::UIDrawList m_CachedDrawList{};
    uint64_t m_CachedDrawListGeneration = ~uint64_t{0};
    we::rhi::Format m_CachedDrawListFormat = we::rhi::Format::Unknown;

    uint32_t m_ActiveFrameSlot = 0;
    uint32_t m_CurrentWidth = 0;
    uint32_t m_CurrentHeight = 0;
    uint32_t m_LastBuiltWidth = 0;
    uint32_t m_LastBuiltHeight = 0;
    uint64_t m_GeometryGeneration = 0;
    uint32_t m_PipelineAuditImageIndex = UINT32_MAX;
    UIFrameStats m_FrameStats;
    we::runtime::uigfx::OverlayRenderContext m_PendingContext{};
    std::mutex m_Mutex;

    float m_LastBuildCpuMs = 0.0f;
    float m_LastSubmitCpuMs = 0.0f;
    UiBuildPhaseTiming m_LastPhaseTiming{};
    bool m_BuiltGeometryThisFrame = false;
    bool m_SubmittedGpuThisFrame = false;
    bool m_UploadedGeometryThisFrame = false;
    bool m_LastSubmissionCacheHit = false;
    bool m_LastSubmissionRebuilt = false;
#pragma warning(pop)
};

} // namespace we::runtime::kindui
