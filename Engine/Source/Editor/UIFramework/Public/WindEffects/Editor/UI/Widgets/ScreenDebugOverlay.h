#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include "KindUI/Core/Widget.h"
#include <string>
#include <vector>

namespace we::editor::panels {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::PaintContext;

// Top-left live readout for WE_SCREEN_DEBUG=1. Pointer-transparent so it
// never eats clicks; paints the latest ScreenRecorder sample as text lines.
class UIFRAMEWORK_API ScreenDebugOverlay : public Widget {
public:
    ScreenDebugOverlay();
    ~ScreenDebugOverlay() override;

    void Tick(float deltaTime) override;
    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    [[nodiscard]] bool IsPointerTransparent() const override { return true; }

private:
    std::vector<std::string> m_Lines;
    float m_RefreshTimer = 0.0f;
};

} // namespace we::editor::panels
