#pragma once

#include "../Core/Widget.hpp"

namespace HouseEngine::UI {

class Spacer : public Widget {
public:
    Spacer() {}
    virtual ~Spacer() = default;

    Size Measure(const Size& availableSize) override {
        m_DesiredSize = Size{ 10000.0f, 10000.0f }; // Massive desired size to trigger Box layout expansion
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
    }

    void Paint(PaintContext& context) override {
        // Spacer is invisible
    }
};

} // namespace HouseEngine::UI
