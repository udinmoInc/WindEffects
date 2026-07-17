#include "KindUI/Core/TextMetrics.h"

namespace we::runtime::kindui {

namespace {

TextMetrics::MeasureFn g_MeasureProvider;

} // namespace

void TextMetrics::SetMeasureProvider(TextMetrics::MeasureFn provider) {
    g_MeasureProvider = std::move(provider);
}

float TextMetrics::MeasureWidth(const std::string_view text, const float fontSize, const bool bold) {
    if (g_MeasureProvider) {
        return g_MeasureProvider(text, fontSize, bold);
    }
    return static_cast<float>(text.size()) * fontSize * 0.5f;
}

float TextMetrics::EstimateWidth(const std::string_view text, const float fontSize) {
    return MeasureWidth(text, fontSize, false);
}

float TextMetrics::CharWidth(const float fontSize) {
    return MeasureWidth("M", fontSize, false);
}

} // namespace we::runtime::kindui
