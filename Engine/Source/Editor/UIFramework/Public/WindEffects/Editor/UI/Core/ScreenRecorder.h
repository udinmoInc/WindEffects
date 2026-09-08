#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::editor::services {

// Instant on-screen + on-disk run debugger.
// TEMP: records by default for direct testing (WE_SCREEN_DEBUG=0 switches
// off). While enabled, every frame appends one JSON line to
// tools/screen/session-<timestamp>.jsonl (flushed regularly) and the
// ScreenDebugOverlay widget shows the same numbers live.
class UIFRAMEWORK_API ScreenRecorder {
public:
    static ScreenRecorder& Get();

    // Named to avoid urlmon.h IsLoggingEnabledA/W macro collision on Windows.
    [[nodiscard]] static bool IsRecordingEnabled();

    // Snapshot EditorPerfStats::Last() + repaint deltas. Cheap: one short
    // line per frame, file flush every kFlushFrames.
    void RecordFrame();

    // Instant event markers (drops, selections...). Written immediately.
    void RecordEvent(const std::string& tag);

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

    // Unscoped enum (not static constexpr): a dllexported class would emit
    // per-TU copies of statics and spam LNK4197 duplicate-export warnings.
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
};

} // namespace we::editor::services
