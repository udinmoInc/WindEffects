#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"

namespace we::runtime::kindui {

/// Thin vertical separator for toolbars and panel chrome rows (UE-style divider).
class KINDUI_API VerticalDivider : public Widget {
public:
    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
};

} // namespace we::runtime::kindui
