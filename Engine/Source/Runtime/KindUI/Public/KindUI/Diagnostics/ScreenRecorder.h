// ==============================================================================
// WindEffects — KindUI — ScreenRecorder
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::kindui {

// Instant on-screen + on-disk run debugger.
// Default ON (WE_SCREEN_DEBUG=0 disables). Every frame appends one JSON line to
// tools/screen/session-<timestamp>.jsonl (flushed regularly) and the
// ScreenDebugOverlay widget shows the same numbers live.
class KINDUI_API ScreenRecorder {
public:
    static ScreenRecorder& Get();

    // Named to avoid urlmon.h IsLoggingEnabledA/W macro collision on Windows.
    [[nodiscard]] static bool IsRecordingEnabled();

    struct FrameMetrics {
        float frameMs = 0.0f;
        float tickMs = 0.0f;
        float layoutMs = 0.0f;
        float uiMs = 0.0f;
        float sceneMs = 0.0f;
        float presentMs = 0.0f;
        float fps = 0.0f;
        uint32_t uiVertices = 0;
        uint32_t uiBatches = 0;
        uint32_t uiIndices = 0;
    };

    // Snapshot frame timings + repaint deltas. Cheap: one short
    // line per frame, file flush every kFlushFrames.
    void RecordFrame(const FrameMetrics& metrics = FrameMetrics{});

    // Instant event markers (drops, selections...). Written immediately.
    void RecordEvent(const std::string& tag);

    // Structured input/debug records for the live Python UI debugger.
    // kind examples: MouseDown, MouseUp, MouseMove, ClickInvoke, Stall, Note
    void RecordInput(
        const char* kind,
        float x,
        float y,
        const std::string& hit,
        const std::string& target,
        const std::string& detail = {});

    void Shutdown();

    // Overlay text lines built from the latest sample.
    [[nodiscard]] std::vector<std::string> BuildOverlayLines() const;

    [[nodiscard]] const std::string& SessionPath() const { return m_SessionPath; }
    [[nodiscard]] uint64_t FrameIndex() const { return m_FrameIndex; }
    [[nodiscard]] const std::string& LastCause() const { return m_LastCause; }

private:
    ScreenRecorder() = default;

    bool EnsureOpen();
    void AppendLine(const std::string& line, bool flush);

    enum : size_t { kRingFrames = 300 };

    struct FrameSample {
        float frameMs = 0.0f;
        float tickMs = 0.0f;
        float layoutMs = 0.0f;
        float uiMs = 0.0f;
        float sceneMs = 0.0f;
        float presentMs = 0.0f;
        float fps = 0.0f;
        uint64_t paintRebuilds = 0;
        uint64_t idleSkips = 0;
    };

    bool m_Enabled = false;
    bool m_EnabledChecked = false;
    bool m_OpenFailed = false;
    std::string m_SessionPath;
    void* m_File = nullptr; // FILE*, kept opaque to avoid stdio in header
    uint64_t m_FrameIndex = 0;
    uint64_t m_LastPaintRebuilds = 0;
    uint64_t m_LastIdleSkips = 0;
    FrameSample m_Ring[kRingFrames]{};
    size_t m_RingHead = 0;
    size_t m_RingCount = 0;
    FrameSample m_Latest{};
    std::string m_LastCause;
    double m_LastInputMs = 0.0;
    double m_LastInputAvgMs = 0.0;
};

} // namespace we::runtime::kindui
