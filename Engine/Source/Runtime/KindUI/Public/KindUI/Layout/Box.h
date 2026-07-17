#pragma once

#include "KindUI/Export.h"
#include "KindUI/Layout/Flex.h"

namespace we::runtime::kindui {

enum class Orientation {
    Horizontal,
    Vertical
};

/// @deprecated Prefer Row/Column (Flex). Retained for Splitter and legacy call sites.
class KINDUI_API Box : public Flex {
public:
    explicit Box(Orientation orientation)
        : Flex(orientation == Orientation::Horizontal ? FlexDirection::Row : FlexDirection::Column) {}

    void SetSpacing(float spacing) { Gap(spacing); }
    void SetPadding(const Margin& padding) { Padding(padding); }
    void SetBackgroundColor(const Color& color) { Background(color); }
};

class KINDUI_API HorizontalBox : public Row {
public:
    HorizontalBox() { Gap(4.0f); }
    void SetSpacing(float spacing) { Gap(spacing); InvalidateLayout(); }
    void SetPadding(const Margin& padding) { Padding(padding); }
    void SetBackgroundColor(const Color& color) { Background(color); }
};

class KINDUI_API VerticalBox : public Column {
public:
    VerticalBox() { Gap(4.0f); }
    void SetSpacing(float spacing) { Gap(spacing); InvalidateLayout(); }
    void SetPadding(const Margin& padding) { Padding(padding); }
    void SetBackgroundColor(const Color& color) { Background(color); }
};

} // namespace we::runtime::kindui