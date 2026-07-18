#include "KindUI/Core/TextMetrics.h"

#include <mutex>

namespace we::runtime::kindui {

namespace {

std::mutex g_MeasureMutex;
TextMetrics::MeasureFn g_MeasureProvider;

} // namespace

void TextMetrics::SetMeasureProvider(TextMetrics::MeasureFn provider) {
    std::scoped_lock lock(g_MeasureMutex);
    g_MeasureProvider = std::move(provider);
}

float TextMetrics::MeasureWidth(const std::string_view text, const float fontSize, const bool bold) {
    TextMetrics::MeasureFn provider;
    {
        std::scoped_lock lock(g_MeasureMutex);
        provider = g_MeasureProvider;
    }
    if (provider) {
        return provider(text, fontSize, bold);
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
