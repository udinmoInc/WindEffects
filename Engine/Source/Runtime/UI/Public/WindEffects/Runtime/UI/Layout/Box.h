#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Layout/Flex.h"

namespace WindEffects::Editor::UI {

enum class Orientation {
    Horizontal,
    Vertical
};

/// @deprecated Prefer Row/Column (Flex). Retained for Splitter and legacy call sites.
class UI_API Box : public Flex {
public:
    explicit Box(Orientation orientation)
        : Flex(orientation == Orientation::Horizontal ? FlexDirection::Row : FlexDirection::Column) {}

    void SetSpacing(float spacing) { Gap(spacing); }
    void SetPadding(const Margin& padding) { Padding(padding); }
    void SetBackgroundColor(const Color& color) { Background(color); }
};

class UI_API HorizontalBox : public Row {
public:
    HorizontalBox() { Gap(4.0f); }
    void SetSpacing(float spacing) { Gap(spacing); InvalidateLayout(); }
    void SetPadding(const Margin& padding) { Padding(padding); }
    void SetBackgroundColor(const Color& color) { Background(color); }
};

class UI_API VerticalBox : public Column {
public:
    VerticalBox() { Gap(4.0f); }
    void SetSpacing(float spacing) { Gap(spacing); InvalidateLayout(); }
    void SetPadding(const Margin& padding) { Padding(padding); }
    void SetBackgroundColor(const Color& color) { Background(color); }
};

} // namespace WindEffects::Editor::UI