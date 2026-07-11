#pragma once

#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"

namespace we::programs::editor::ActorsPanelLayout {

inline float ContentPadH() {
    return WindEffects::Editor::UI::PanelChrome::PanelPaddingH();
}

inline float ChevronSize() {
    return WindEffects::Editor::UI::ResolveThemeMetric(WindEffects::Editor::UI::ThemeToken::IconSizeTree);
}

inline float IconSize() {
    return ChevronSize();
}

inline float RowHeight() {
    return WindEffects::Editor::UI::PanelChrome::ListRowHeight();
}

inline float CategoryHeight() {
    return WindEffects::Editor::UI::PanelChrome::CategoryHeaderHeight();
}

inline float SearchHeight() {
    return WindEffects::Editor::UI::PanelChrome::SearchHeight();
}

inline float SearchRowHeight() {
    return SearchHeight() + WindEffects::Editor::UI::ResolveThemeMetric(WindEffects::Editor::UI::ThemeToken::Space1);
}

inline float ItemIconX(float contentX) {
    return contentX + ContentPadH() + ChevronSize()
        + WindEffects::Editor::UI::ResolveThemeMetric(WindEffects::Editor::UI::ThemeToken::Space2);
}

inline float LabelX(float contentX) {
    return ItemIconX(contentX) + IconSize()
        + WindEffects::Editor::UI::ResolveThemeMetric(WindEffects::Editor::UI::ThemeToken::Space2);
}

inline float StarIconX(float contentX, float contentWidth) {
    return contentX + contentWidth - ContentPadH() - IconSize();
}

inline float CategoryGap() {
    return WindEffects::Editor::UI::ResolveThemeMetric(WindEffects::Editor::UI::ThemeToken::Space2);
}

inline float ToolbarIconSize() {
    return WindEffects::Editor::UI::ResolveThemeMetric(WindEffects::Editor::UI::ThemeToken::IconButtonSize);
}

} // namespace we::programs::editor::ActorsPanelLayout
