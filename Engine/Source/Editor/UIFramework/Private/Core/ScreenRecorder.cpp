#include "WindEffects/Editor/UI/Core/ScreenRecorder.h"
#include "WindEffects/Editor/UI/Core/EditorPerfStats.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Profiling/PaintCauseLog.h"
#include "Core/Paths.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#if defined(_WIN32)
#include <share.h>
#endif

using ::we::runtime::kindui::UIRepaintGate;

namespace we::editor::services {

namespace {

constexpr uint64_t kFlushFrames = 120;

bool EnvOn(const char* name) {
    const char* v = std::getenv(name);
    return v != nullptr && v[0] != '\0' && v[0] != '0';
}

std::string SessionStamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d%02d%02d-%02d%02d%02d",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);
    return std::string(buf);
}

} // namespace

ScreenRecorder& ScreenRecorder::Get() {
    static ScreenRecorder instance;
    return instance;
}

bool ScreenRecorder::IsRecordingEnabled() {
    // TEMP (direct testing): record by default; set WE_SCREEN_DEBUG=0 to
    // switch off. Revert to env-only (`return EnvOn("WE_SCREEN_DEBUG");`)
    // once the overlay workflow is confirmed.
    const char* v = std::getenv("WE_SCREEN_DEBUG");
    if (v != nullptr && v[0] != '\0') {
        return v[0] != '0';
    }
    return true;
}

bool ScreenRecorder::EnsureOpen() {
    if (m_File != nullptr) {
        return true;
    }
    if (m_OpenFailed) {
        return false;
    }
    if (!m_EnabledChecked) {
        m_Enabled = IsRecordingEnabled();
        m_EnabledChecked = true;
    }
    if (!m_Enabled) {
        return false;
    }
    std::error_code ec;
    std::filesystem::path dir;
    // Resolve the repo root (exe dir can be anywhere: staged build, Binaries,
    // VS debugger...). Fall back to the working directory like before.
    if (const auto repo = we::core::PathService::FindRepositoryRoot(
            we::core::PathService::Get().ExecutableDirectory())) {
        dir = *repo / "tools" / "screen";
    } else {
        dir = std::filesystem::path("tools") / "screen";
    }
    std::filesystem::create_directories(dir, ec);
    m_SessionPath = (dir / ("session-" + SessionStamp() + ".jsonl")).string();
    FILE* f = nullptr;
#if defined(_WIN32)
    // Shared-read so tools/screen_view.py can tail the live session while
    // the editor still holds it open. Writes stay exclusive to us.
    f = _fsopen(m_SessionPath.c_str(), "w", _SH_DENYWR);
#else
    f = std::fopen(m_SessionPath.c_str(), "w");
#endif
    if (f == nullptr) {
        m_OpenFailed = true;
        return false;
    }
    m_File = f;
    m_LastPaintRebuilds = UIRepaintGate::PaintRebuildCount();
    m_LastIdleSkips = UIRepaintGate::IdleSkipCount();
    return true;
}

void ScreenRecorder::AppendLine(const std::string& line, bool flush) {
    if (!EnsureOpen()) {
        return;
    }
    FILE* f = static_cast<FILE*>(m_File);
    std::fputs(line.c_str(), f);
    std::fputc('\n', f);
    if (flush) {
        std::fflush(f);
    }
}

void ScreenRecorder::RecordFrame() {
    if (!EnsureOpen()) {
        return;
    }
    const auto& s = EditorPerfStats::Get().Last();
    const uint64_t paints = UIRepaintGate::PaintRebuildCount();
    const uint64_t skips = UIRepaintGate::IdleSkipCount();

    FrameSample sample{};
    sample.frameMs = s.frameMs;
    sample.tickMs = s.tickMs;
    sample.layoutMs = s.layoutMs;
    sample.uiMs = s.uiBuildMs;
    sample.sceneMs = s.sceneMs;
    sample.presentMs = s.presentMs;
    sample.fps = EditorPerfStats::Get().AverageFps();
    sample.paintRebuilds = paints - m_LastPaintRebuilds;
    sample.idleSkips = skips - m_LastIdleSkips;
    m_LastPaintRebuilds = paints;
    m_LastIdleSkips = skips;
    m_Latest = sample;

    m_Ring[m_RingHead] = sample;
    m_RingHead = (m_RingHead + 1) % kRingFrames;
    if (m_RingCount < kRingFrames) {
        ++m_RingCount;
    }
    ++m_FrameIndex;

    std::ostringstream line;
    line << "{\"t\":\"frame\",\"f\":" << m_FrameIndex
         << ",\"ms\":" << sample.frameMs
         << ",\"tick\":" << sample.tickMs
         << ",\"layout\":" << sample.layoutMs
         << ",\"ui\":" << sample.uiMs
         << ",\"scene\":" << sample.sceneMs
         << ",\"present\":" << sample.presentMs
         << ",\"fps\":" << sample.fps
         << ",\"paints\":" << sample.paintRebuilds
         << ",\"skips\":" << sample.idleSkips
         << ",\"verts\":" << s.uiVertices
         << ",\"batches\":" << s.uiBatches
         << ",\"idx\":" << (s.uiOpaqueIndices + s.uiAlphaIndices) << "}";
    AppendLine(line.str(), (m_FrameIndex % kFlushFrames) == 0);

    // Paint-cause attribution: who requested paint since the last frame.
    // Recorded with exact caller (function+line via DbgHelp on Windows).
    for (auto& cause : we::runtime::kindui::PaintCauseLog::Get().Drain()) {
        std::ostringstream cline;
        cline << "{\"t\":\"cause\",\"f\":" << m_FrameIndex
              << ",\"cms\":" << cause.ms << ",\"kind\":\"" << cause.kind << "\",\"caller\":\"";
        for (char c : cause.caller) {
            if (c == '"' || c == '\\') {
                cline << '\\';
            }
            cline << (c == '\n' ? ' ' : c);
        }
        cline << "\"}";
        AppendLine(cline.str(), false);
        m_LastCause = cause.caller;
    }
}

void ScreenRecorder::RecordEvent(const std::string& tag) {
    if (!EnsureOpen()) {
        return;
    }
    std::ostringstream line;
    line << "{\"t\":\"event\",\"f\":" << m_FrameIndex << ",\"tag\":\"";
    for (char c : tag) {
        if (c == '"' || c == '\\') {
            line << '\\';
        }
        line << (c == '\n' ? ' ' : c);
    }
    line << "\"}";
    AppendLine(line.str(), true);
}

void ScreenRecorder::Shutdown() {
    if (m_File != nullptr) {
        std::fflush(static_cast<FILE*>(m_File));
        std::fclose(static_cast<FILE*>(m_File));
        m_File = nullptr;
    }
}

std::vector<std::string> ScreenRecorder::BuildOverlayLines() const {
    const FrameSample& s = m_Latest;
    float worst = 0.0f;
    float sum = 0.0f;
    for (size_t i = 0; i < m_RingCount; ++i) {
        worst = (m_Ring[i].frameMs > worst) ? m_Ring[i].frameMs : worst;
        sum += m_Ring[i].frameMs;
    }
    const float avg = m_RingCount > 0 ? sum / static_cast<float>(m_RingCount) : 0.0f;

    // Quantized to whole units on purpose: the overlay change-gates its
    // repaints, so jittery decimals must not count as content changes.
    std::ostringstream f, w, t;
    f << "scr fps " << std::fixed << std::setprecision(0) << s.fps
      << " ms " << s.frameMs
      << " avg " << avg << " worst " << worst;
    w << std::fixed << std::setprecision(0)
      << "tick " << s.tickMs << " layout " << s.layoutMs << " ui " << s.uiMs;
    t << std::fixed << std::setprecision(0)
      << "scene " << s.sceneMs << " present " << s.presentMs
      << " paints " << s.paintRebuilds << " skips " << s.idleSkips;
    std::string cause = m_LastCause;
    if (cause.size() > 52) {
        cause = cause.substr(cause.size() - 52);
    }
    return {
        std::string("REC ") + std::filesystem::path(m_SessionPath).filename().string(),
        f.str(),
        w.str(),
        t.str(),
        std::string("cause ") + (cause.empty() ? "-" : cause),
    };
}

} // namespace we::editor::services
