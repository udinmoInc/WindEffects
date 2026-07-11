#pragma once

#include "Text/Core/FontTypes.h"
#include "Text/Core/Types.h"
#include "Text/Export.h"
#include "Text/Layout/TextLayoutEngine.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace we::runtime::text::diagnostics {

struct DiagnosticMessage {
    std::string category;
    std::string message;
    std::string context;
};

struct FontDebuggerEntry {
    FontHandle handle = kInvalidFontHandle;
    std::string family;
    std::string assetPath;
    size_t glyphCount = 0;
    uint32_t atlasWidth = 0;
    uint32_t atlasHeight = 0;
};

struct DiagnosticsSnapshot {
    std::vector<FontDebuggerEntry> loadedFonts;
    std::vector<DiagnosticMessage> messages;
    size_t cacheHits = 0;
    size_t cacheMisses = 0;
    size_t drawCalls = 0;
    size_t verticesRendered = 0;
    size_t textureMemoryBytes = 0;
    size_t missingGlyphReports = 0;
};

class TEXT_API IFontDiagnostics {
public:
    virtual ~IFontDiagnostics() = default;
    virtual void Log(DiagnosticMessage message) = 0;
    virtual void ReportMissingGlyph(Codepoint codepoint, FontHandle attempted, FontHandle resolved) = 0;
    [[nodiscard]] virtual DiagnosticsSnapshot CaptureSnapshot() const = 0;
    virtual void ResetFrameStats() = 0;
};

[[nodiscard]] TEXT_API std::unique_ptr<IFontDiagnostics> CreateFontDiagnostics();

} // namespace we::runtime::text::diagnostics
