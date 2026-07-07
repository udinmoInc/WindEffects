#pragma once

#include "Application/Export.h"

#include "Core/Widget.h"

namespace we::UI {

class APPLICATION_API Spacer : public Widget {
public:
    Spacer();
    ~Spacer() override;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
};

} // namespace we::UI
