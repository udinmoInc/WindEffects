// ==============================================================================
// WindEffects — KindUI — PanelToolbarRow
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/UI/Flex.h"
#include "KindUI/UI/DesignSystemControls.h"
#include "KindUI/Theme/ThemeAccess.h"
#include "KindUI/Theme/DesignToken.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace we::runtime::kindui {

/// Reusable panel toolbar: search field + trailing icon buttons in a flex row.
/// Derives all geometry from parent constraints; no manual X/Y placement.
class KINDUI_API PanelToolbarRow : public Row {
public:
    explicit PanelToolbarRow(std::string searchPlaceholder = "Search...");

    void SetSearchText(std::string text);
    [[nodiscard]] const std::string& GetSearchText() const;

    void SetOnSearchChanged(std::function<void(const std::string&)> callback);
    void AddSeparator();
    void AddLeadingIconButton(WindIconRef icon, std::function<void()> onClicked);
    void AddIconButton(WindIconRef icon, std::function<void()> onClicked);

    /// Adds search + icon children to the flex row. Safe to call once after configuration.
    void Finalize();
    [[nodiscard]] Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void SetDrawBottomBorder(bool drawBorder) { m_DrawBottomBorder = drawBorder; }
    void SetDrawBorder(bool drawBorder) { m_DrawBottomBorder = drawBorder; }
    void SetShowBorder(bool showBorder) { m_DrawBottomBorder = showBorder; }
    [[nodiscard]] bool GetDrawBottomBorder() const { return m_DrawBottomBorder; }

    void SetToolbarPadding(float left, float top, float right, float bottom) { Padding(Margin{ left, top, right, bottom }); }

    [[nodiscard]] std::shared_ptr<IconButton> GetIconButton(size_t index) const;
    [[nodiscard]] std::shared_ptr<SearchBoxControl> GetSearchBox() const { return m_SearchBox; }

protected:
    void EnsureBuilt();

private:

    std::string m_SearchPlaceholder;
    std::shared_ptr<SearchBoxControl> m_SearchBox;
    std::vector<std::shared_ptr<Widget>> m_LeadingItems;
    std::vector<std::shared_ptr<Widget>> m_TrailingItems;
    std::vector<std::shared_ptr<IconButton>> m_IconButtons;
    std::vector<std::function<void()>> m_IconCallbacks;
    bool m_Built = false;
    bool m_DrawBottomBorder = false;
};

} // namespace we::runtime::kindui
