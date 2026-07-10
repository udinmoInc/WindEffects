#include "Text/Diagnostics/FontDiagnostics.h"

#include <mutex>

namespace we::runtime::text::diagnostics {

class FontDiagnostics final : public IFontDiagnostics {
public:
    void Log(DiagnosticMessage message) override
    {
        std::lock_guard lock(m_Mutex);
        m_Messages.push_back(std::move(message));
    }

    void ReportMissingGlyph(
        const Codepoint codepoint,
        const FontHandle attempted,
        const FontHandle resolved) override
    {
        DiagnosticMessage message;
        message.category = "MissingGlyph";
        message.message = "Missing glyph U+" + std::to_string(static_cast<uint32_t>(codepoint));
        message.context = "attempted=" + std::to_string(attempted) + " resolved=" + std::to_string(resolved);
        Log(std::move(message));
        ++m_MissingGlyphReports;
    }

    DiagnosticsSnapshot CaptureSnapshot() const override
    {
        std::lock_guard lock(m_Mutex);
        DiagnosticsSnapshot snapshot;
        snapshot.messages = m_Messages;
        snapshot.loadedFonts = m_LoadedFonts;
        snapshot.cacheHits = m_CacheHits;
        snapshot.cacheMisses = m_CacheMisses;
        snapshot.drawCalls = m_DrawCalls;
        snapshot.verticesRendered = m_VerticesRendered;
        snapshot.textureMemoryBytes = m_TextureMemoryBytes;
        snapshot.missingGlyphReports = m_MissingGlyphReports;
        return snapshot;
    }

    void ResetFrameStats() override
    {
        std::lock_guard lock(m_Mutex);
        m_DrawCalls = 0;
        m_VerticesRendered = 0;
    }

    void RegisterFont(const FontDebuggerEntry& entry)
    {
        std::lock_guard lock(m_Mutex);
        m_LoadedFonts.push_back(entry);
    }

    void RecordDrawStats(const size_t drawCalls, const size_t vertices)
    {
        std::lock_guard lock(m_Mutex);
        m_DrawCalls += drawCalls;
        m_VerticesRendered += vertices;
    }

private:
    mutable std::mutex m_Mutex;
    std::vector<DiagnosticMessage> m_Messages;
    std::vector<FontDebuggerEntry> m_LoadedFonts;
    size_t m_CacheHits = 0;
    size_t m_CacheMisses = 0;
    size_t m_DrawCalls = 0;
    size_t m_VerticesRendered = 0;
    size_t m_TextureMemoryBytes = 0;
    size_t m_MissingGlyphReports = 0;
};

std::unique_ptr<IFontDiagnostics> CreateFontDiagnostics()
{
    return std::make_unique<FontDiagnostics>();
}

} // namespace we::runtime::text::diagnostics
