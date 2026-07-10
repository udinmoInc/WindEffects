#pragma once

#include "Rendering/Text/ShapedGlyph.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct FT_FaceRec_;

namespace we::UI::Text {

// Unicode text shaping via HarfBuzz + FreeType (kerning, ligatures, bidi-ready buffer).
class APPLICATION_API HarfBuzzTextLayout {
public:
    HarfBuzzTextLayout();
    ~HarfBuzzTextLayout();

    HarfBuzzTextLayout(const HarfBuzzTextLayout&) = delete;
    HarfBuzzTextLayout& operator=(const HarfBuzzTextLayout&) = delete;

    bool Initialize(FT_FaceRec_* primaryFace, FT_FaceRec_* fallbackFace);
    void Shutdown();

    bool IsReady() const;

    bool Shape(std::string_view utf8Text, std::vector<ShapedGlyph>& outGlyphs) const;
    float MeasureWidth(std::string_view utf8Text) const;

    const TextLayoutMetrics& GetMetrics() const { return m_Metrics; }
    const std::string& GetPrimaryFontPath() const { return m_PrimaryFontPath; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
    TextLayoutMetrics m_Metrics{};
    std::string m_PrimaryFontPath;
};

} // namespace we::UI::Text
