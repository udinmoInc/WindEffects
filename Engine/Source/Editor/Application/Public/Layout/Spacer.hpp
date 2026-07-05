#pragma once

#include "Application/Export.hpp"

#include "Core/Widget.hpp"

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
