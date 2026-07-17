#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Widget.h"

namespace we::runtime::kindui {

class KINDUI_API Spacer : public Widget {
public:
    Spacer();
    ~Spacer() override;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
};

} // namespace we::runtime::kindui
