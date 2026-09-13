// ==============================================================================
// WindEffects — EditorShell — EditorPerfStats
// Public API surface for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <cstdint>
#include <string>

namespace we::editor::services {

// Lightweight frame instrumentation. Enable with WE_EDITOR_PERF=1 for once/sec logs.
struct EDITORSHELL_API EditorPerfSample {
    float frameMs = 0.0f;
    float tickMs = 0.0f;
    float layoutMs = 0.0f;
    float rhiPrepareMs = 0.0f;
    float uiBuildMs = 0.0f;
    float uiOnlyMs = 0.0f;
    float uiBuildCpuMs = 0.0f;
    float uiSubmitCpuMs = 0.0f;
    float sceneMs = 0.0f;
    float presentMs = 0.0f;
    uint32_t uiVertices = 0;
    uint32_t uiBatches = 0;
    uint32_t uiOpaqueBatches = 0;
    uint32_t uiAlphaBatches = 0;
    uint32_t uiOpaqueIndices = 0;
    uint32_t uiAlphaIndices = 0;
    uint32_t uiBuiltFrames = 0;
    uint32_t uiSubmitFrames = 0;
    uint32_t uiUploadFrames = 0;
    uint64_t uiRebuilds = 0;
    uint64_t uiSkips = 0;
    uint64_t uiLayoutRebuilds = 0;
    uint64_t uiPaintRebuilds = 0;
    uint64_t uiIdleSkips = 0;
    uint64_t uiSubCacheHits = 0;
    uint64_t uiSubCacheMisses = 0;
    uint64_t uiSubRebuilds = 0;
    uint64_t uiSubInvalidations = 0;
    uint32_t uiSubCacheHitFrames = 0;
    uint32_t uiSubRebuildFrames = 0;
};

class EDITORSHELL_API EditorPerfStats {
public:
    static EditorPerfStats& Get();

    void BeginFrame();
    void Mark(const char* stage); // "tick" | "layout" | "rhi" | "ui" | "scene" | "present"
    void EndFrame(
        uint32_t uiVertices,
        uint32_t uiBatches,
        uint32_t uiOpaqueBatches = 0,
        uint32_t uiAlphaBatches = 0,
        uint32_t uiOpaqueIndices = 0,
        uint32_t uiAlphaIndices = 0,
        float uiBuildCpuMs = 0.0f,
        float uiSubmitCpuMs = 0.0f,
        bool uiBuilt = false,
        bool uiSubmitted = false,
        bool uiUploaded = false,
        bool uiSubmissionCacheHit = false,
        bool uiSubmissionRebuilt = false,
        uint64_t uiSubCacheHits = 0,
        uint64_t uiSubCacheMisses = 0,
        uint64_t uiSubRebuilds = 0,
        uint64_t uiSubInvalidations = 0);

    [[nodiscard]] const EditorPerfSample& Last() const { return m_Last; }
    [[nodiscard]] float AverageFps() const { return m_AvgFps; }

    // Named to avoid urlmon.h IsLoggingEnabledA/W macro collision on Windows.
    [[nodiscard]] static bool IsPerfLoggingEnabled();

private:
    EditorPerfStats() = default;

    double NowMs() const;

    EditorPerfSample m_Last{};
    EditorPerfSample m_Accum{};
    uint32_t m_AccumFrames = 0;
    double m_FrameStartMs = 0.0;
    double m_StageStartMs = 0.0;
    double m_LastLogMs = 0.0;
    float m_AvgFps = 0.0f;
};

} // namespace we::editor::services
