#include "KindUI/Core/TextMetrics.h"

#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

namespace we::runtime::kindui {

float TextMetrics::CharWidth(float fontSize) {
    const float ratio = ResolveMetric(MetricToken::TextCharWidthRatio);
    return fontSize * ratio;
}

float TextMetrics::EstimateWidth(std::string_view text, float fontSize) {
    return static_cast<float>(text.size()) * CharWidth(fontSize);
}

} // namespace we::runtime::kindui
