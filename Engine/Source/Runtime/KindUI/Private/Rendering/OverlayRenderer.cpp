// ==============================================================================
// WindEffects — KindUI — OverlayRenderer
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Rendering/OverlayRenderer.h"

#include "KindUI/Rendering/IconRenderer.h"
#include "KindUI/Profiling/UiGeometryDebug.h"
#include "KindUI/Profiling/UiColorDebug.h"
#include "KindUI/Profiling/UiColorPipelineDiagnostic.h"
#include "KindUI/Profiling/UiColorCompositionDiagnostic.h"
#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Rendering/Icons/IconManager.h"
#include "KindUI/Rendering/TextUIService.h"
#include "KindUI/Rendering/UIWidgetAdapter.h"
#include "KindUI/Rendering/UIStateManager.h"
#include "KindUI/Rendering/UiGpuUpload.h"
#include "KindUI/Profiling/UiPathDiagnostics.h"
#include "KindUI/Profiling/UiInputLatencyAudit.h"
#include "KindUI/Profiling/UiBuildPhaseTiming.h"
#include "Rendering/UiImmediateRenderer.h"

#include "Core/AssetRegistry.h"
#include "Core/FrameCounter.h"
#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Core/Paths.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Core/Widget.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace we::runtime::kindui {
namespace {

[[nodiscard]] we::rhi::UIDrawList BuildDrawList(
    const std::vector<UIVertex2>& vertices,
    const std::vector<uint32_t>& indices,
    const std::vector<UIRenderBatch>& batches,
    we::rhi::Format targetFormat,
    uint32_t width,
    uint32_t height)
{
    we::rhi::UIDrawList list{};
    list.targetWidth = width;
    list.targetHeight = height;
    list.vertices.resize(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        auto& dst = list.vertices[i];
        const auto& src = vertices[i];
        std::memcpy(dst.position, src.position, sizeof(dst.position));
        std::memcpy(dst.uv, src.uv, sizeof(dst.uv));
        ColorSpace::WriteGpuVertexColorForTarget(
            targetFormat,
            Color{src.color[0], src.color[1], src.color[2], src.color[3]},
            dst.color);
        std::memcpy(dst.sdfRect, src.sdfRect, sizeof(dst.sdfRect));
        std::memcpy(dst.sdfParams, src.sdfParams, sizeof(dst.sdfParams));
    }
    list.indices = indices;
    list.batches.reserve(batches.size());
    for (const auto& batch : batches) {
        we::rhi::UIDrawBatch out{};
        out.texture = batch.textureSet;
        out.indexCount = batch.indexCount;
        out.firstIndex = batch.firstIndex;
        out.vertexOffset = batch.vertexOffset;
        std::memcpy(out.scissor, batch.scissor, sizeof(out.scissor));
        out.stencilRef = batch.stencilRef;
        out.isText = batch.isText;
        out.opaqueReplace = batch.opaqueReplace;
        out.atlasWidth = batch.atlasWidth;
        out.atlasHeight = batch.atlasHeight;
        out.msdfPixelRange = batch.msdfPixelRange;
        list.batches.push_back(out);
    }
    return list;
}

} // namespace

OverlayRenderer::OverlayRenderer() = default;

OverlayRenderer::~OverlayRenderer() {
    Shutdown();
}

bool OverlayRenderer::Init(we::rhi::IRHIDevice* device, we::rhi::Format swapchainFormat, uint32_t maxFramesInFlight) {
    if (!device) {
        return false;
    }
    m_RHIDevice = device;
    m_SwapchainFormat = swapchainFormat;
    m_MaxFramesInFlight = maxFramesInFlight ? maxFramesInFlight : 2;

    m_UIImmediate = std::make_unique<UiImmediateRenderer>();
    if (!m_UIImmediate->Init(device, swapchainFormat, m_MaxFramesInFlight)) {
        WE_LOG_WARN(we::LogCategory::Startup,
            "OverlayRenderer: UiImmediateRenderer Init failed; UI will not record GPU draws.");
        m_UIImmediate.reset();
    }

    if (m_UIImmediate) {
        m_DummyDescriptorSet = static_cast<uint64_t>(m_UIImmediate->GetDummyTexture());
        m_DummySampler = static_cast<uint64_t>(m_UIImmediate->GetDefaultSampler());
    } else {
        m_DummyDescriptorSet = 1;
        m_DummySampler = 1;
    }

    m_GpuUpload = std::make_unique<UiGpuUpload>();
    m_GpuUpload->Init(device);
    m_TextUIService = std::make_unique<TextUIService>();
    (void)m_TextUIService->Initialize(this);
    m_IconRenderer = std::make_unique<IconRenderer>();
    m_IconManager = std::make_unique<IconManager>();

    const auto windIconsCandidates = we::core::PathService::Get().IconCandidates(
        std::filesystem::path("WindIcons"));
    const auto windIconsRoot = we::core::PathService::FindExisting(windIconsCandidates);
    if (windIconsRoot) {
        if (m_IconManager->Init(this, *windIconsRoot)) {
            m_IconRenderer->SetIconManager(m_IconManager.get());
        } else {
            WE_LOG_WARN(we::LogCategory::Startup, "OverlayRenderer: IconManager init failed.");
        }
    } else {
        WE_LOG_WARN(we::LogCategory::Startup, "OverlayRenderer: WindIcons folder not found.");
    }

    m_WidgetAdapter = std::make_unique<UIWidgetAdapter>();
    m_WidgetAdapter->Initialize(this);
    m_StateManager = std::make_unique<UIStateManager>();

    WE_LOG_INFO(we::LogCategory::Startup,
        m_UIImmediate
            ? "OverlayRenderer initialized (geometry + UiImmediateRenderer/IRHI)."
            : "OverlayRenderer initialized (CPU geometry only; GPU UI unavailable).");
    return true;
}

void OverlayRenderer::Shutdown() {
    if (m_WidgetAdapter) {
        m_WidgetAdapter->Shutdown();
    }
    if (m_GpuUpload) {
        m_GpuUpload->Shutdown();
    }
    if (m_UIImmediate) {
        m_UIImmediate->Shutdown();
        m_UIImmediate.reset();
    }
    m_RHIDevice = nullptr;
    m_Vertices.clear();
    m_Indices.clear();
    m_Batches.clear();
}

void OverlayRenderer::SetTargetExtent(uint32_t width, uint32_t height) {
    m_CurrentWidth = width;
    m_CurrentHeight = height;
    m_FrameStats.width = width;
    m_FrameStats.height = height;
}

void OverlayRenderer::InvalidateGpuSubmissionCache() {
    if (m_UIImmediate) {
        m_UIImmediate->InvalidateGpuSubmissionCache();
    }
}

void OverlayRenderer::SetSwapchainFormat(we::rhi::Format format) {
    m_SwapchainFormat = format;
    if (m_UIImmediate) {
        m_UIImmediate->SetSwapchainFormat(format);
    }
}

uint64_t OverlayRenderer::SubmissionCacheHitCount() const {
    return m_UIImmediate ? m_UIImmediate->SubmissionCacheHitCount() : 0;
}

uint64_t OverlayRenderer::SubmissionCacheMissCount() const {
    return m_UIImmediate ? m_UIImmediate->SubmissionCacheMissCount() : 0;
}

uint64_t OverlayRenderer::SubmissionRebuildCount() const {
    return m_UIImmediate ? m_UIImmediate->SubmissionRebuildCount() : 0;
}

uint64_t OverlayRenderer::SubmissionInvalidationCount() const {
    return m_UIImmediate ? m_UIImmediate->SubmissionInvalidationCount() : 0;
}

void OverlayRenderer::RenderUI(const std::shared_ptr<Widget>& root, uint32_t frameSlot) {
    m_ActiveFrameSlot = frameSlot;
    m_LastBuildCpuMs = 0.0f;
    m_LastPhaseTiming = {};
    m_BuiltGeometryThisFrame = false;
    m_UploadedGeometryThisFrame = false;
    const uint64_t frameNumber = we::runtime::core::FrameCounter::GetFrameNumber();
    const uint32_t width = m_CurrentWidth;
    const uint32_t height = m_CurrentHeight;

    if (UiColorDebug::IsEnabled() || UiColorDebug::IsSemanticAuditEnabled()) {
        UiColorDebug::Get().BeginFrame();
    }
    if (UiGeometryDebug::IsEnabled()) {
        UiGeometryDebug::Get().BeginFrame();
    }

    if (!root || width == 0 || height == 0) {
        m_Vertices.clear();
        m_Indices.clear();
        m_Batches.clear();
        if (UiColorDebug::IsEnabled() || UiColorDebug::IsSemanticAuditEnabled()) {
            UiColorDebug::Get().EndFrame();
        }
        if (UiGeometryDebug::IsEnabled()) {
            UiGeometryDebug::Get().EndFrame();
        }
        return;
    }

    if (UiColorCompositionDiagnostic::IsEnabled()) {
        UiColorCompositionDiagnostic::Get().BeginFrame(width, height);
    }

    const bool sizeChanged = width != m_LastBuiltWidth || height != m_LastBuiltHeight;
    const bool compositionAudit = UiColorCompositionDiagnostic::IsEnabled()
        && !UiColorCompositionDiagnostic::Get().HasCompleted();
    const bool forceRebuild = frameNumber <= 3 || sizeChanged || m_Vertices.empty() || compositionAudit;
    // Layout Measure/Arrange is owned by the host (Editor SyncViewport / WeLauncher SyncLayout).
    // Peek only — do not ConsumeNeedsLayout here (host is the sole consumer).
    const bool needsLayout = forceRebuild || UIRepaintGate::PeekNeedsLayout();
    const bool needsPaint = forceRebuild
        || UIRepaintGate::ConsumeNeedsPaint()
        || needsLayout
        || compositionAudit;

    if (!needsLayout && !needsPaint) {
        m_FrameStats.width = width;
        m_FrameStats.height = height;
        return;
    }

    const auto buildStart = std::chrono::steady_clock::now();
    if (needsPaint) {
        UiInputLatencyAudit::Get().OnUiBuild();
        Widget::ResetDiagnostics();
            if (m_WidgetAdapter) {
                m_WidgetAdapter->ResetDiagnostics();
                m_WidgetAdapter->ProcessWidget(root, width, height, needsLayout);
                m_WidgetAdapter->SwapGeometry(m_Vertices, m_Indices, m_Batches);
                m_LastPhaseTiming = m_WidgetAdapter->LastPhaseTiming();
            }
        m_LastBuiltWidth = width;
        m_LastBuiltHeight = height;
        ++m_GeometryGeneration;
        m_BuiltGeometryThisFrame = true;
    }

    m_FrameStats.vertices = static_cast<uint32_t>(m_Vertices.size());
    m_FrameStats.indices = static_cast<uint32_t>(m_Indices.size());
    m_FrameStats.batches = static_cast<uint32_t>(m_Batches.size());
    m_FrameStats.drawCalls = m_FrameStats.batches;
    m_FrameStats.opaqueBatches = 0;
    m_FrameStats.alphaBatches = 0;
    m_FrameStats.opaqueIndices = 0;
    m_FrameStats.alphaIndices = 0;
    for (const auto& batch : m_Batches) {
        if (batch.opaqueReplace) {
            ++m_FrameStats.opaqueBatches;
            m_FrameStats.opaqueIndices += batch.indexCount;
        } else {
            ++m_FrameStats.alphaBatches;
            m_FrameStats.alphaIndices += batch.indexCount;
        }
    }
    m_FrameStats.width = width;
    m_FrameStats.height = height;
    m_LastBuildCpuMs = static_cast<float>(
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - buildStart).count());

    static const bool buildProfile = []() {
        const char* v = std::getenv("WE_UI_BUILD_PROFILE");
        return v != nullptr && v[0] != '\0' && v[0] != '0';
    }();
    if (buildProfile && m_BuiltGeometryThisFrame) {
        static double s_LastLogMs = 0.0;
        const double nowMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        if (nowMs - s_LastLogMs >= 1000.0) {
            s_LastLogMs = nowMs;
            const auto& p = m_LastPhaseTiming;
            HE_INFO(
                std::string("[UiBuildProfile] total=") + std::to_string(p.totalMs) +
                "ms clear=" + std::to_string(p.clearMs) +
                " layout=" + std::to_string(p.layoutMs) +
                " paint=" + std::to_string(p.paintMs) +
                " drawgen=" + std::to_string(p.drawgenMs) +
                " text=" + std::to_string(p.textMs) +
                " clearDirty=" + std::to_string(p.clearDirtyMs) +
                " cmds=" + std::to_string(p.paintCommands) +
                " textCmds=" + std::to_string(p.textCommands) +
                " rectCmds=" + std::to_string(p.rectCommands) +
                " verts=" + std::to_string(p.vertices) +
                " batches=" + std::to_string(p.batches) +
                " ranLayout=" + (p.ranLayout ? "1" : "0") +
                " retain=" + (p.paintRetention ? "1" : "0") +
                " painted=" + std::to_string(p.subtreesPainted) +
                " replayed=" + std::to_string(p.subtreesReplayed) +
                " replayCmds=" + std::to_string(p.commandsReplayed) +
                " wall=" + std::to_string(m_LastBuildCpuMs));
        }
    }

    if (UiColorDebug::IsEnabled() || UiColorDebug::IsSemanticAuditEnabled()) {
        UiColorDebug::Get().EndFrame();
    }
    if (UiGeometryDebug::IsEnabled()) {
        UiGeometryDebug::Get().EndFrame();
    }
}

void OverlayRenderer::BeginOverlayPass(const we::runtime::uigfx::OverlayRenderContext& context) {
    m_PendingContext = context;
    if (context.targetExtent.width != 0 && context.targetExtent.height != 0) {
        m_CurrentWidth = context.targetExtent.width;
        m_CurrentHeight = context.targetExtent.height;
    }
}

void OverlayRenderer::EndOverlayPass(const we::runtime::uigfx::OverlayRenderContext& context) {
    m_LastSubmitCpuMs = 0.0f;
    m_SubmittedGpuThisFrame = false;
    m_UploadedGeometryThisFrame = false;
    m_LastSubmissionCacheHit = false;
    m_LastSubmissionRebuilt = false;
    if (!m_UIImmediate || m_Vertices.empty() || m_Batches.empty()) {
        return;
    }
    if (!context.cmd) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            "OverlayRenderer::EndOverlayPass: no command list available; skipping UI draw.");
        return;
    }

    const auto submitStart = std::chrono::steady_clock::now();

    we::rhi::FramePresentParams params{};
    params.commandList = context.cmd;
    params.targetView = context.targetView;
    params.colorTarget = context.colorTarget;
    params.extent = context.targetExtent;
    params.imageIndex = context.imageIndex;

    m_UIImmediate->BeginFrame(params);
    const we::rhi::Format targetFormat = context.targetFormat != we::rhi::Format::Unknown
        ? context.targetFormat
        : m_SwapchainFormat;
    // Rebuild the CPU draw list only when geometry or target format/size changed.
    // GPU upload already skips unchanged geometryGeneration inside UiImmediateRenderer.
    if (m_CachedDrawListGeneration != m_GeometryGeneration
        || m_CachedDrawListFormat != targetFormat
        || m_CachedDrawList.targetWidth != m_CurrentWidth
        || m_CachedDrawList.targetHeight != m_CurrentHeight) {
        m_CachedDrawList = BuildDrawList(
            m_Vertices, m_Indices, m_Batches, targetFormat, m_CurrentWidth, m_CurrentHeight);
        m_CachedDrawListGeneration = m_GeometryGeneration;
        m_CachedDrawListFormat = targetFormat;
    }
    m_UIImmediate->SubmitDrawList(m_CachedDrawList, m_ActiveFrameSlot, m_GeometryGeneration);
    m_UploadedGeometryThisFrame = m_UIImmediate->LastSubmitUploadedGeometry();
    m_LastSubmissionCacheHit = m_UIImmediate->LastSubmissionCacheHit();
    m_LastSubmissionRebuilt = m_UIImmediate->LastSubmissionRebuilt();
    UiInputLatencyAudit::Get().OnRenderSubmit();
    m_UIImmediate->EndFrame();
    m_SubmittedGpuThisFrame = true;
    m_LastSubmitCpuMs = static_cast<float>(
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - submitStart).count());

    if (UiColorPipelineDiagnostic::IsEnabled()
        && !UiColorCompositionDiagnostic::IsEnabled()
        && context.cmd) {
        UiColorPipelineDiagnostic::Get().ScheduleFramebufferReadback(
            m_RHIDevice,
            context.cmd,
            context.colorTarget,
            targetFormat,
            m_CurrentWidth,
            m_CurrentHeight);
    }
    if (UiColorCompositionDiagnostic::IsEnabled()
        && !UiColorCompositionDiagnostic::Get().HasCompleted()
        && context.cmd) {
        UiColorCompositionDiagnostic::Get().ScheduleFramebufferReadback(
            m_RHIDevice,
            context.cmd,
            context.colorTarget,
            targetFormat,
            m_CurrentWidth,
            m_CurrentHeight);
    }
}

void OverlayRenderer::SetPipelineAuditImageIndex(uint32_t imageIndex) {
    m_PipelineAuditImageIndex = imageIndex;
}

we::rhi::RHIDescriptorSetHandle OverlayRenderer::RegisterTexture(
    we::rhi::RHITextureViewHandle imageView,
    we::rhi::RHISamplerHandle sampler)
{
    if (m_UIImmediate) {
        return m_UIImmediate->RegisterTexture(imageView, sampler);
    }
    return static_cast<we::rhi::RHIDescriptorSetHandle>(++m_DummyDescriptorSet);
}

void OverlayRenderer::UpdateTexture(
    we::rhi::RHIDescriptorSetHandle descriptorSet,
    we::rhi::RHITextureViewHandle imageView,
    we::rhi::RHISamplerHandle sampler)
{
    if (m_UIImmediate) {
        m_UIImmediate->UpdateTexture(descriptorSet, imageView, sampler);
    }
}

void OverlayRenderer::UnregisterTexture(we::rhi::RHIDescriptorSetHandle descriptorSet) {
    if (m_UIImmediate) {
        m_UIImmediate->UnregisterTexture(descriptorSet);
    }
}

we::rhi::RHIDescriptorSetHandle OverlayRenderer::UploadRgbaTexture(
    uint32_t width,
    uint32_t height,
    std::span<const uint8_t> rgba,
    bool linearFilter,
    bool srgb)
{
    if (m_UIImmediate) {
        return m_UIImmediate->UploadRgbaTexture(width, height, rgba, linearFilter, srgb);
    }
    return we::rhi::RHIDescriptorSetHandle::Invalid;
}

TextUIService* OverlayRenderer::GetTextUIService() const { return m_TextUIService.get(); }
IconRenderer* OverlayRenderer::GetIconRenderer() const { return m_IconRenderer.get(); }
IconManager* OverlayRenderer::GetIconManager() const { return m_IconManager.get(); }

} // namespace we::runtime::kindui

// kindui-perf-rebuild-token
