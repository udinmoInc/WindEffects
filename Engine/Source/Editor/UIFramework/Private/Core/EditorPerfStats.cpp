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

void EditorPerfStats::EndFrame(uint32_t uiVertices, uint32_t uiBatches) {
    const double now = NowMs();
    m_Last.frameMs = static_cast<float>(now - m_FrameStartMs);
    m_Last.uiVertices = uiVertices;
    m_Last.uiBatches = uiBatches;
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
    m_Accum.sceneMs += m_Last.sceneMs;
    m_Accum.presentMs += m_Last.presentMs;
    m_Accum.uiVertices += m_Last.uiVertices;
    m_Accum.uiBatches += m_Last.uiBatches;
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
        " uiOnly=" + std::to_string((m_Accum.tickMs + m_Accum.layoutMs + m_Accum.uiBuildMs) / n) + "ms" +
        " scene=" + std::to_string(m_Accum.sceneMs / n) + "ms" +
        " present=" + std::to_string(m_Accum.presentMs / n) + "ms" +
        " verts=" + std::to_string(static_cast<uint32_t>(m_Accum.uiVertices / m_AccumFrames)) +
        " batches=" + std::to_string(static_cast<uint32_t>(m_Accum.uiBatches / m_AccumFrames)) +
        " uiLayoutRebuild=" + std::to_string(UIRepaintGate::LayoutRebuildCount()) +
        " uiPaintRebuild=" + std::to_string(UIRepaintGate::PaintRebuildCount()) +
        " uiIdleSkip=" + std::to_string(UIRepaintGate::IdleSkipCount()) +
        " uiRebuild=" + std::to_string(UIRepaintGate::RebuildCount()) +
        " uiSkip=" + std::to_string(UIRepaintGate::SkipCount()));

    m_Accum = {};
    m_AccumFrames = 0;
    m_LastLogMs = now;
}

} // namespace we::editor::services
