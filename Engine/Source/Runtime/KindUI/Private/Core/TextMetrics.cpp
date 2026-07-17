#include "KindUI/Core/TextMetrics.h"

#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeToken.h"

namespace we::runtime::kindui {

float TextMetrics::CharWidth(float fontSize) {
    const float ratio = ResolveThemeMetric(ThemeToken::TextCharWidthRatio);
    return fontSize * ratio;
}

float TextMetrics::EstimateWidth(std::string_view text, float fontSize) {
    return static_cast<float>(text.size()) * CharWidth(fontSize);
}

} // namespace we::runtime::kindui
