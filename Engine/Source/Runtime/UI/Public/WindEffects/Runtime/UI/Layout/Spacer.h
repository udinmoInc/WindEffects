#pragma once

#include "WindEffects/Runtime/UI/Export.h"

#include "WindEffects/Runtime/UI/Core/Widget.h"

namespace WindEffects::Editor::UI {

class UI_API Spacer : public Widget {
public:
    Spacer();
    ~Spacer() override;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
};

} // namespace WindEffects::Editor::UI
