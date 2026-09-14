// ==============================================================================
// WindEffects — KindUI — OverlayRenderer
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Host/OverlayRenderer.h"

#include "KindUI/Host/IconRenderer.h"
#include "KindUI/Diagnostics/UiGeometryDebug.h"
#include "Profiling/UiColorDebug.h"
#include "KindUI/Diagnostics/UiColorPipelineDiagnostic.h"
#include "KindUI/Diagnostics/UiColorCompositionDiagnostic.h"
#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Host/IconManager.h"
#include "Rendering/TextUIService.h"
#include "Rendering/UIWidgetAdapter.h"
#include "Rendering/UIStateManager.h"
#include "Rendering/UiGpuUpload.h"
#include "KindUI/Diagnostics/UiPathDiagnostics.h"
#include "KindUI/Diagnostics/UiInputLatencyAudit.h"
#include "KindUI/Diagnostics/UiBuildPhaseTiming.h"
#include "Rendering/UiImmediateRenderer.h"

#include "Core/AssetRegistry.h"
#include "Core/FrameCounter.h"
#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Core/Paths.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Core/UIDirtyRegionTracker.h"
#include "KindUI/Core/UIResourceResidency.h"
#include "KindUI/Core/Widget.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace we::runtime::kindui {
namespace {

static_assert(sizeof(UIVertex2) == sizeof(we::rhi::UIVertex), "UIVertex2 must match RHI UIVertex");
static_assert(alignof(UIVertex2) == alignof(we::rhi::UIVertex), "UIVertex2 align must match RHI UIVertex");

/// Rebuild cached draw list into persistent storage (capacity retained). Color convert only.
void BuildDrawList(
    we::rhi::UIDrawList& list,
    const std::vector<UIVertex2>& vertices,
    const std::vector<uint32_t>& indices,
    const std::vector<UIRenderBatch>& batches,
    we::rhi::Format targetFormat,
    uint32_t width,
    uint32_t height)
{
    list.targetWidth = width;
    list.targetHeight = height;

    list.vertices.resize(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        we::rhi::UIVertex& dst = list.vertices[i];
        const UIVertex2& src = vertices[i];
        // Layout-identical: copy then overwrite color for target encoding.
        std::memcpy(&dst, &src, sizeof(we::rhi::UIVertex));
        ColorSpace::WriteGpuVertexColorForTarget(
            targetFormat,
            Color{src.color[0], src.color[1], src.color[2], src.color[3]},
            dst.color);
    }

    list.indices.assign(indices.begin(), indices.end());

    list.batches.clear();
    if (list.batches.capacity() < batches.size()) {
        list.batches.reserve(batches.size());
    }
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
    // Idempotent: drop any prior GPU/UI state before (re)building.
    Shutdown();

    m_RHIDevice = device;
    m_SwapchainFormat = swapchainFormat;
    m_MaxFramesInFlight = maxFramesInFlight ? maxFramesInFlight : 2;

    WE_LOG_INFO(we::LogCategory::Startup, "OverlayRenderer: UiImmediateRenderer...");
    we::runtime::core::Logger::Flush();
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

    WE_LOG_INFO(we::LogCategory::Startup, "OverlayRenderer: UiGpuUpload...");
    we::runtime::core::Logger::Flush();
    m_GpuUpload = std::make_unique<UiGpuUpload>();
    m_GpuUpload->Init(device);

    WE_LOG_INFO(we::LogCategory::Startup, "OverlayRenderer: TextUIService...");
    we::runtime::core::Logger::Flush();
    m_TextUIService = std::make_unique<TextUIService>();
    if (!m_TextUIService->Initialize(this)) {
        WE_LOG_ERROR(we::LogCategory::Startup, "OverlayRenderer: TextUIService Initialize failed.");
        m_TextUIService.reset();
    }

    WE_LOG_INFO(we::LogCategory::Startup, "OverlayRenderer: IconManager...");
    we::runtime::core::Logger::Flush();
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
    // Tear down consumers of descriptor sets before the owner (UiImmediate).
    // Reset unique_ptrs here so member-destructor order cannot Unregister into a dead renderer.
    if (m_IconRenderer) {
        m_IconRenderer->SetIconManager(nullptr);
        m_IconRenderer->Shutdown();
        m_IconRenderer.reset();
    }
    if (m_IconManager) {
        m_IconManager->Shutdown();
        m_IconManager.reset();
    }
    if (m_TextUIService) {
        m_TextUIService->Shutdown();
        m_TextUIService.reset();
    }
    if (m_WidgetAdapter) {
        m_WidgetAdapter->Shutdown();
        m_WidgetAdapter.reset();
    }
    if (m_StateManager) {
        m_StateManager.reset();
    }
    if (m_GpuUpload) {
        m_GpuUpload->Shutdown();
        m_GpuUpload.reset();
    }

    m_Vertices.clear();
    m_Indices.clear();
    m_Batches.clear();
    m_CachedDrawList = {};
    m_CachedDrawListGeneration = ~uint64_t{0};
    m_DummyDescriptorSet = 0;
    m_DummySampler = 0;

    if (m_UIImmediate) {
        m_UIImmediate->Shutdown();
        m_UIImmediate.reset();
    }
    m_RHIDevice = nullptr;
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

uint64_t OverlayRenderer::GetGeometryVertexCapacityBytes() const {
    return static_cast<uint64_t>(m_Vertices.capacity()) * static_cast<uint64_t>(sizeof(UIVertex2));
}

uint64_t OverlayRenderer::GetGeometryIndexCapacityBytes() const {
    return static_cast<uint64_t>(m_Indices.capacity()) * sizeof(uint32_t);
}

uint64_t OverlayRenderer::GetGeometryBatchCapacityBytes() const {
    return static_cast<uint64_t>(m_Batches.capacity()) * static_cast<uint64_t>(sizeof(UIRenderBatch));
}

uint64_t OverlayRenderer::GetDrawCommandCapacityBytes() const {
    return m_WidgetAdapter ? m_WidgetAdapter->GetDrawCommandCapacityBytes() : 0;
}

uint64_t OverlayRenderer::GetGpuVertexCapacityBytes() const {
    return m_UIImmediate ? m_UIImmediate->GetGpuVertexCapacityBytes() : 0;
}

uint64_t OverlayRenderer::GetGpuIndexCapacityBytes() const {
    return m_UIImmediate ? m_UIImmediate->GetGpuIndexCapacityBytes() : 0;
}

const UiGpuPathStats* OverlayRenderer::GetGpuPathStats() const {
    return m_UIImmediate ? &m_UIImmediate->GetGpuPathStats() : nullptr;
}

size_t OverlayRenderer::GetTextMeasureCacheEntryCount() const {
    return m_TextUIService ? m_TextUIService->MeasureCacheEntryCount() : 0;
}

uint64_t OverlayRenderer::GetTextMeasureCacheBytes() const {
    return m_TextUIService ? m_TextUIService->EstimateMeasureCacheBytes() : 0;
}

uint32_t OverlayRenderer::GetFontAtlasPageCount() const {
    return m_TextUIService ? m_TextUIService->FontAtlasPageCount() : 0;
}

uint64_t OverlayRenderer::GetFontAtlasCpuBytes() const {
    return m_TextUIService ? m_TextUIService->EstimateFontAtlasCpuBytes() : 0;
}

size_t OverlayRenderer::GetIconTextureCacheEntryCount() const {
    return m_IconManager ? m_IconManager->TextureCacheEntryCount() : 0;
}

uint64_t OverlayRenderer::GetIconTextureCacheBytes() const {
    return m_IconManager ? m_IconManager->EstimatedGpuBytes() : 0;
}

uint32_t OverlayRenderer::GetSubmissionCacheSlotCount() const {
    return m_UIImmediate ? m_UIImmediate->SubmissionCacheSlotCount() : 0;
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

    auto& residency = UIResourceResidency::Get();
    const uint32_t fif = m_UIImmediate ? m_UIImmediate->FramesInFlight() : 2u;
    residency.BeginFrame(frameNumber, fif);

    if (m_IconManager) {
        m_IconManager->OnFrame(frameNumber, this, root);
    }
    if (m_TextUIService) {
        m_TextUIService->OnResidencyTick();
    }
    // Merge icon residency into the global snapshot (text tick wrote geom/atlas).
    {
        const auto& prior = residency.Stats();
        residency.SetResidentSnapshot(
            m_IconManager ? static_cast<uint32_t>(m_IconManager->TextureCacheEntryCount()) : 0,
            m_IconManager ? m_IconManager->EstimatedGpuBytes() : 0,
            prior.residentTextGeomCount,
            prior.residentTextGeomCpuBytes,
            prior.residentGlyphCount,
            prior.residentAtlasCpuBytes);
    }

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
    if (forceRebuild || sizeChanged) {
        UIDirtyRegionTracker::Get().MarkFullDirty();
    }
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
                const bool geometryReused = m_WidgetAdapter->LastPhaseTiming().geometryReused;
                m_WidgetAdapter->SwapGeometry(m_Vertices, m_Indices, m_Batches);
                m_LastPhaseTiming = m_WidgetAdapter->LastPhaseTiming();
                // Reused drawgen output keeps prior GPU buffers / submission cache valid.
                if (!geometryReused) {
                    ++m_GeometryGeneration;
                }
            } else {
                ++m_GeometryGeneration;
            }
        m_LastBuiltWidth = width;
        m_LastBuiltHeight = height;
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
            const UiGpuPathStats* gpu = GetGpuPathStats();
            HE_INFO(
                std::string("[UiBuildProfile] total=") + std::to_string(p.totalMs) +
                "ms clear=" + std::to_string(p.clearMs) +
                " layout=" + std::to_string(p.layoutMs) +
                " paint=" + std::to_string(p.paintMs) +
                " coalesce=" + std::to_string(p.coalesceMs) +
                " drawgen=" + std::to_string(p.drawgenMs) +
                " batchCoalesce=" + std::to_string(p.batchCoalesceMs) +
                " text=" + std::to_string(p.textMs) +
                " clearDirty=" + std::to_string(p.clearDirtyMs) +
                " cmds=" + std::to_string(p.paintCommands) +
                "->" + std::to_string(p.commandsAfterCoalesce) +
                " dropped=" + std::to_string(p.commandsDropped) +
                " rectMerge=" + std::to_string(p.rectsMerged) +
                " clipNorm=" + std::to_string(p.clipsNormalized) +
                " iconCluster=" + std::to_string(p.iconsClustered) +
                " textCluster=" + std::to_string(p.textsClustered) +
                " textCmds=" + std::to_string(p.textCommands) +
                " rectCmds=" + std::to_string(p.rectCommands) +
                " verts=" + std::to_string(p.vertices) +
                " idx=" + std::to_string(p.indices) +
                " batches=" + std::to_string(p.batchesAfterDrawgen) +
                "->" + std::to_string(p.batches) +
                " batchMerge=" + std::to_string(p.batchesMerged) +
                " texSw=" + std::to_string(p.textureSwitches) +
                " clips=" + std::to_string(p.clipRectCount) +
                " globalBatch=" + (p.globalBatchEnabled ? "1" : "0") +
                " ranLayout=" + (p.ranLayout ? "1" : "0") +
                " retain=" + (p.paintRetention ? "1" : "0") +
                " painted=" + std::to_string(p.subtreesPainted) +
                " replayed=" + std::to_string(p.subtreesReplayed) +
                " replayCmds=" + std::to_string(p.commandsReplayed) +
                " dirtyR=" + std::to_string(p.dirtyRegionCount) +
                " dirtyCov=" + std::to_string(p.dirtyCoverage) +
                " dirtyFull=" + (p.dirtyFull ? "1" : "0") +
                " geomReuse=" + (p.geometryReused ? "1" : "0") +
                " layM=" + std::to_string(p.layoutMeasureRan) +
                "/" + std::to_string(p.layoutMeasureRan + p.layoutMeasureSkipped) +
                " layA=" + std::to_string(p.layoutArrangeRan) +
                "/" + std::to_string(p.layoutArrangeRan + p.layoutArrangeSkipped) +
                " layFull=" + std::to_string(p.layoutFullPasses) +
                " wall=" + std::to_string(m_LastBuildCpuMs) +
                " submitCpu=" + std::to_string(m_LastSubmitCpuMs) +
                " opaqueB=" + std::to_string(m_FrameStats.opaqueBatches) +
                " alphaB=" + std::to_string(m_FrameStats.alphaBatches) +
                " upload=" + (m_UploadedGeometryThisFrame ? "1" : "0") +
                " subHit=" + (m_LastSubmissionCacheHit ? "1" : "0") +
                " gpuUp=" + std::to_string(gpu ? gpu->geometryUploadCount : 0) +
                " gpuBytes=" + std::to_string(gpu ? gpu->geometryUploadBytes : 0) +
                " gpuSkip=" + std::to_string(gpu ? gpu->geometryUploadSkipCount : 0) +
                " gpuHashSkip=" + std::to_string(gpu ? gpu->geometryContentHashSkipCount : 0) +
                " gpuBufCreate=" + std::to_string(gpu ? gpu->bufferCreateCount : 0) +
                " gpuBufRealloc=" + std::to_string(gpu ? gpu->bufferReallocCount : 0) +
                " gpuUploadMs=" + std::to_string(gpu ? gpu->lastGeometryUploadCpuMs : 0.0f) +
                " gpuVB=" + std::to_string(GetGpuVertexCapacityBytes()) +
                " gpuIB=" + std::to_string(GetGpuIndexCapacityBytes()) +
                " texCreate=" + std::to_string(gpu ? gpu->textureCreateCount : 0) +
                " texUpdate=" + std::to_string(gpu ? gpu->textureUpdateCount : 0));
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
        BuildDrawList(
            m_CachedDrawList,
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

void OverlayRenderer::RetireTexture(we::rhi::RHIDescriptorSetHandle descriptorSet) {
    if (m_UIImmediate) {
        m_UIImmediate->RetireTexture(descriptorSet);
    }
    UIResourceResidency::Get().NoteDeferredRelease();
}

void OverlayRenderer::PrepareForResourceEviction(const std::shared_ptr<Widget>& root) {
    // Drop any CPU/GPU caches that may still reference retiring descriptor sets.
    if (root) {
        root->ReleaseRetainedPaintSubtree();
    }
    m_CachedDrawList = {};
    m_CachedDrawListGeneration = ~uint64_t{0};
    ++m_GeometryGeneration;
    InvalidateGpuSubmissionCache();
    UIDirtyRegionTracker::Get().MarkFullDirty();
    UIRepaintGate::RequestPaintReason("ResourceEviction");
    UIResourceResidency::Get().PinThroughGpuHorizon();
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

bool OverlayRenderer::UpdateRgbaTexturePixels(
    we::rhi::RHIDescriptorSetHandle set,
    uint32_t width,
    uint32_t height,
    std::span<const uint8_t> rgba)
{
    if (m_UIImmediate) {
        return m_UIImmediate->UpdateRgbaTexturePixels(set, width, height, rgba);
    }
    return false;
}

TextUIService* OverlayRenderer::GetTextUIService() const { return m_TextUIService.get(); }
IconRenderer* OverlayRenderer::GetIconRenderer() const { return m_IconRenderer.get(); }
IconManager* OverlayRenderer::GetIconManager() const { return m_IconManager.get(); }

} // namespace we::runtime::kindui
