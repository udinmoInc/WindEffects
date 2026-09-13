// ==============================================================================
// WindEffects — EditorShell — EditorPerfStats
// Internal implementation for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "WindEffects/Editor/UI/Core/EditorPerfStats.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "Core/Logger.h"

#include <chrono>
#include <cstdlib>
#include <cstring>

using ::we::runtime::kindui::UIRepaintGate;

namespace we::editor::services {

namespace {

bool EnvEnabled(const char* name) {
    const char* v = std::getenv(name);
    return v != nullptr && v[0] != '\0' && v[0] != '0';
}

} // namespace

EditorPerfStats& EditorPerfStats::Get() {
    static EditorPerfStats instance;
    return instance;
}

bool EditorPerfStats::IsPerfLoggingEnabled() {
    return EnvEnabled("WE_EDITOR_PERF");
}

double EditorPerfStats::NowMs() const {
    using clock = std::chrono::steady_clock;
    const auto now = clock::now().time_since_epoch();
    return std::chrono::duration<double, std::milli>(now).count();
}

void EditorPerfStats::BeginFrame() {
    m_FrameStartMs = NowMs();
    m_StageStartMs = m_FrameStartMs;
    m_Last = {};
}

void EditorPerfStats::Mark(const char* stage) {
    const double now = NowMs();
    const float delta = static_cast<float>(now - m_StageStartMs);
    if (stage) {
        if (std::strcmp(stage, "tick") == 0) {
            m_Last.tickMs = delta;
        } else if (std::strcmp(stage, "layout") == 0) {
            m_Last.layoutMs = delta;
        } else if (std::strcmp(stage, "rhi") == 0) {
            m_Last.rhiPrepareMs = delta;
        } else if (std::strcmp(stage, "ui") == 0) {
            m_Last.uiBuildMs = delta;
        } else if (std::strcmp(stage, "scene") == 0) {
            m_Last.sceneMs = delta;
        } else if (std::strcmp(stage, "present") == 0) {
            m_Last.presentMs = delta;
        }
    }
    m_StageStartMs = now;
}

void EditorPerfStats::EndFrame(
    uint32_t uiVertices,
    uint32_t uiBatches,
    uint32_t uiOpaqueBatches,
    uint32_t uiAlphaBatches,
    uint32_t uiOpaqueIndices,
    uint32_t uiAlphaIndices,
    float uiBuildCpuMs,
    float uiSubmitCpuMs,
    bool uiBuilt,
    bool uiSubmitted,
    bool uiUploaded,
    bool uiSubmissionCacheHit,
    bool uiSubmissionRebuilt,
    uint64_t uiSubCacheHits,
    uint64_t uiSubCacheMisses,
    uint64_t uiSubRebuilds,
    uint64_t uiSubInvalidations) {
    const double now = NowMs();
    m_Last.frameMs = static_cast<float>(now - m_FrameStartMs);
    m_Last.uiVertices = uiVertices;
    m_Last.uiBatches = uiBatches;
    m_Last.uiOpaqueBatches = uiOpaqueBatches;
    m_Last.uiAlphaBatches = uiAlphaBatches;
    m_Last.uiOpaqueIndices = uiOpaqueIndices;
    m_Last.uiAlphaIndices = uiAlphaIndices;
    m_Last.uiBuildCpuMs = uiBuildCpuMs;
    m_Last.uiSubmitCpuMs = uiSubmitCpuMs;
    m_Last.uiBuiltFrames = uiBuilt ? 1u : 0u;
    m_Last.uiSubmitFrames = uiSubmitted ? 1u : 0u;
    m_Last.uiUploadFrames = uiUploaded ? 1u : 0u;
    m_Last.uiSubCacheHitFrames = uiSubmissionCacheHit ? 1u : 0u;
    m_Last.uiSubRebuildFrames = uiSubmissionRebuilt ? 1u : 0u;
    m_Last.uiSubCacheHits = uiSubCacheHits;
    m_Last.uiSubCacheMisses = uiSubCacheMisses;
    m_Last.uiSubRebuilds = uiSubRebuilds;
    m_Last.uiSubInvalidations = uiSubInvalidations;
    m_Last.uiRebuilds = UIRepaintGate::RebuildCount();
    m_Last.uiSkips = UIRepaintGate::SkipCount();
    m_Last.uiLayoutRebuilds = UIRepaintGate::LayoutRebuildCount();
    m_Last.uiPaintRebuilds = UIRepaintGate::PaintRebuildCount();
    m_Last.uiIdleSkips = UIRepaintGate::IdleSkipCount();

    m_Accum.frameMs += m_Last.frameMs;
    m_Accum.tickMs += m_Last.tickMs;
    m_Accum.layoutMs += m_Last.layoutMs;
    m_Accum.rhiPrepareMs += m_Last.rhiPrepareMs;
    m_Accum.uiBuildMs += m_Last.uiBuildMs;
    m_Accum.uiBuildCpuMs += m_Last.uiBuildCpuMs;
    m_Accum.uiSubmitCpuMs += m_Last.uiSubmitCpuMs;
    m_Accum.sceneMs += m_Last.sceneMs;
    m_Accum.presentMs += m_Last.presentMs;
    m_Accum.uiVertices += m_Last.uiVertices;
    m_Accum.uiBatches += m_Last.uiBatches;
    m_Accum.uiOpaqueBatches += m_Last.uiOpaqueBatches;
    m_Accum.uiAlphaBatches += m_Last.uiAlphaBatches;
    m_Accum.uiOpaqueIndices += m_Last.uiOpaqueIndices;
    m_Accum.uiAlphaIndices += m_Last.uiAlphaIndices;
    m_Accum.uiBuiltFrames += m_Last.uiBuiltFrames;
    m_Accum.uiSubmitFrames += m_Last.uiSubmitFrames;
    m_Accum.uiUploadFrames += m_Last.uiUploadFrames;
    m_Accum.uiSubCacheHitFrames += m_Last.uiSubCacheHitFrames;
    m_Accum.uiSubRebuildFrames += m_Last.uiSubRebuildFrames;
    ++m_AccumFrames;

    if (m_Last.frameMs > 0.001f) {
        const float fps = 1000.0f / m_Last.frameMs;
        m_AvgFps = m_AvgFps > 0.0f ? (m_AvgFps * 0.9f + fps * 0.1f) : fps;
    }

    if (!IsPerfLoggingEnabled() || m_AccumFrames == 0) {
        return;
    }

    if (now - m_LastLogMs < 1000.0) {
        return;
    }

    const float n = static_cast<float>(m_AccumFrames);
    HE_INFO(
        std::string("[EditorPerf] fps=") + std::to_string(m_AvgFps) +
        " frame=" + std::to_string(m_Accum.frameMs / n) + "ms" +
        " tick=" + std::to_string(m_Accum.tickMs / n) + "ms" +
        " layout=" + std::to_string(m_Accum.layoutMs / n) + "ms" +
        " rhi=" + std::to_string(m_Accum.rhiPrepareMs / n) + "ms" +
        " ui=" + std::to_string(m_Accum.uiBuildMs / n) + "ms" +
        " uiBuildCpu=" + std::to_string(m_Accum.uiBuildCpuMs / n) + "ms" +
        " uiSubmitCpu=" + std::to_string(m_Accum.uiSubmitCpuMs / n) + "ms" +
        " uiOnly=" + std::to_string((m_Accum.tickMs + m_Accum.layoutMs + m_Accum.uiBuildMs) / n) + "ms" +
        " scene=" + std::to_string(m_Accum.sceneMs / n) + "ms" +
        " present=" + std::to_string(m_Accum.presentMs / n) + "ms" +
        " verts=" + std::to_string(static_cast<uint32_t>(m_Accum.uiVertices / m_AccumFrames)) +
        " batches=" + std::to_string(static_cast<uint32_t>(m_Accum.uiBatches / m_AccumFrames)) +
        " built=" + std::to_string(m_Accum.uiBuiltFrames) + "/" + std::to_string(m_AccumFrames) +
        " submit=" + std::to_string(m_Accum.uiSubmitFrames) + "/" + std::to_string(m_AccumFrames) +
        " upload=" + std::to_string(m_Accum.uiUploadFrames) + "/" + std::to_string(m_AccumFrames) +
        " subHit=" + std::to_string(m_Accum.uiSubCacheHitFrames) + "/" + std::to_string(m_AccumFrames) +
        " subRebuild=" + std::to_string(m_Accum.uiSubRebuildFrames) + "/" + std::to_string(m_AccumFrames) +
        " subHits=" + std::to_string(uiSubCacheHits) +
        " subMiss=" + std::to_string(uiSubCacheMisses) +
        " subRebuilds=" + std::to_string(uiSubRebuilds) +
        " subInv=" + std::to_string(uiSubInvalidations) +
        " opaqueBatches=" + std::to_string(static_cast<uint32_t>(m_Accum.uiOpaqueBatches / m_AccumFrames)) +
        " alphaBatches=" + std::to_string(static_cast<uint32_t>(m_Accum.uiAlphaBatches / m_AccumFrames)) +
        " opaqueIdx=" + std::to_string(static_cast<uint32_t>(m_Accum.uiOpaqueIndices / m_AccumFrames)) +
        " alphaIdx=" + std::to_string(static_cast<uint32_t>(m_Accum.uiAlphaIndices / m_AccumFrames)) +
        " uiLayoutRebuild=" + std::to_string(UIRepaintGate::LayoutRebuildCount()) +
        " uiPaintRebuild=" + std::to_string(UIRepaintGate::PaintRebuildCount()) +
        " uiIdleSkip=" + std::to_string(UIRepaintGate::IdleSkipCount()) +
        " uiRebuild=" + std::to_string(UIRepaintGate::RebuildCount()) +
        " uiSkip=" + std::to_string(UIRepaintGate::SkipCount()) +
        " invLayout=" + UIRepaintGate::LastLayoutReason() +
        " invPaint=" + UIRepaintGate::LastPaintReason() +
        " invLayoutN=" + std::to_string(UIRepaintGate::LayoutReasonCount()) +
        " invPaintN=" + std::to_string(UIRepaintGate::PaintReasonCount()));

    m_Accum = {};
    m_AccumFrames = 0;
    m_LastLogMs = now;
}

} // namespace we::editor::services
