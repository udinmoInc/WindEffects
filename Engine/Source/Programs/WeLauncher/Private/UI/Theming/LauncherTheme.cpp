#include "UI/Theming/LauncherTheme.h"
#include "KindUI/Theming/GraphiteDarkTheme.h"

namespace we::programs::welauncher {

we::runtime::kindui::Color LauncherTheme::ResolveColor(we::runtime::kindui::ColorToken token) const {
    return GraphiteDarkTheme::ResolveColor(token);
}

float LauncherTheme::ResolveMetric(we::runtime::kindui::MetricToken token) const {
    switch (token) {
    case we::runtime::kindui::MetricToken::ButtonHeight: return 34.0f;
    case we::runtime::kindui::MetricToken::ControlHeightCompact: return 28.0f;
    case we::runtime::kindui::MetricToken::ControlHeightLarge: return 40.0f;
    case we::runtime::kindui::MetricToken::FormRowHeight: return 40.0f;
    case we::runtime::kindui::MetricToken::MenuItemHeight: return 28.0f;
    case we::runtime::kindui::MetricToken::PageMargin: return 16.0f;
    case we::runtime::kindui::MetricToken::SectionGap: return 24.0f;
    case we::runtime::kindui::MetricToken::CardPadding: return 12.0f;
    case we::runtime::kindui::MetricToken::ContentGap: return 12.0f;
    case we::runtime::kindui::MetricToken::FormRowGap: return 8.0f;
    case we::runtime::kindui::MetricToken::LabelHintGap: return 2.0f;
    case we::runtime::kindui::MetricToken::CornerRadiusSmall: return 8.0f;
    case we::runtime::kindui::MetricToken::CornerRadiusMedium: return 8.0f;
    case we::runtime::kindui::MetricToken::ButtonPaddingHorizontal: return 12.0f;
    case we::runtime::kindui::MetricToken::ListRowHeight: return 44.0f;
    case we::runtime::kindui::MetricToken::SearchBoxHeight: return 34.0f;
    case we::runtime::kindui::MetricToken::HeaderControlHeight: return 34.0f;
    case we::runtime::kindui::MetricToken::TextSizeTitle: return 22.0f;
    case we::runtime::kindui::MetricToken::TextSizeHeader: return 15.0f;
    case we::runtime::kindui::MetricToken::TextSizeBody: return 13.0f;
    case we::runtime::kindui::MetricToken::TextSizeSmall: return 12.0f;
    case we::runtime::kindui::MetricToken::TextSizeCaption: return 11.0f;
    case we::runtime::kindui::MetricToken::TextSizeToolbar: return 12.0f;
    case we::runtime::kindui::MetricToken::IconSizeToolbar:
    case we::runtime::kindui::MetricToken::IconSizeNavigation: return 24.0f;
    case we::runtime::kindui::MetricToken::IconSizeSearch: return 16.0f;
    case we::runtime::kindui::MetricToken::HoverAnimationDamping: return 14.0f;
    case we::runtime::kindui::MetricToken::PressAnimationDamping: return 18.0f;
    case we::runtime::kindui::MetricToken::TitleBarHeight: return 36.0f;
    default:
        return GraphiteDarkTheme::ResolveMetric(token);
    }
}

} // namespace we::programs::welauncher
