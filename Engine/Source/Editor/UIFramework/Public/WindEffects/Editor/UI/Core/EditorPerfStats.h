#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <cstdint>
#include <string>

namespace we::editor::ui {

// Lightweight frame instrumentation. Enable with WE_EDITOR_PERF=1 for once/sec logs.
struct UIFRAMEWORK_API EditorPerfSample {
    float frameMs = 0.0f;
    float tickMs = 0.0f;
    float layoutMs = 0.0f;
    float uiBuildMs = 0.0f;
    float sceneMs = 0.0f;
    float presentMs = 0.0f;
    uint32_t uiVertices = 0;
    uint32_t uiBatches = 0;
    uint64_t uiRebuilds = 0;
    uint64_t uiSkips = 0;
};

class UIFRAMEWORK_API EditorPerfStats {
public:
    static EditorPerfStats& Get();

    void BeginFrame();
    void Mark(const char* stage); // "tick" | "layout" | "ui" | "scene" | "present"
    void EndFrame(uint32_t uiVertices, uint32_t uiBatches);

    [[nodiscard]] const EditorPerfSample& Last() const { return m_Last; }
    [[nodiscard]] float AverageFps() const { return m_AvgFps; }

    [[nodiscard]] static bool IsLoggingEnabled();

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

} // namespace we::editor::ui
