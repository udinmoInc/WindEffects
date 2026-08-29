#pragma once

#include "ContentBrowser/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/PaintContext.h"

namespace we::editor::contentbrowser {

/// Explorer tree column header row (Item Label / Type) as its own layout slot.
class CONTENTBROWSER_API TreeColumnHeader : public we::runtime::kindui::Widget {
public:
    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
};

} // namespace we::editor::contentbrowser
