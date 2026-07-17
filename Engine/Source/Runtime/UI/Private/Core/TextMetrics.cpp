#include "WindEffects/Runtime/UI/Core/TextMetrics.h"

#include "WindEffects/Runtime/UI/Theming/ThemeAccess.h"
#include "WindEffects/Runtime/UI/Theming/ThemeToken.h"

namespace WindEffects::Editor::UI {

float TextMetrics::CharWidth(float fontSize) {
    const float ratio = ResolveThemeMetric(ThemeToken::TextCharWidthRatio);
    return fontSize * ratio;
}

float TextMetrics::EstimateWidth(std::string_view text, float fontSize) {
    return static_cast<float>(text.size()) * CharWidth(fontSize);
}

} // namespace WindEffects::Editor::UI
