#pragma once

#include "KindUI/Export.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"

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
    void AddIconButton(const char* icon, std::function<void()> onClicked);

    /// Adds search + icon children to the flex row. Safe to call once after configuration.
    void Finalize();
    [[nodiscard]] Size Measure(const Size& availableSize) override;

    [[nodiscard]] std::shared_ptr<IconButton> GetIconButton(size_t index) const;
    [[nodiscard]] std::shared_ptr<SearchBoxControl> GetSearchBox() const { return m_SearchBox; }

protected:
    void EnsureBuilt();

private:

    std::string m_SearchPlaceholder;
    std::shared_ptr<SearchBoxControl> m_SearchBox;
    std::vector<std::shared_ptr<IconButton>> m_IconButtons;
    std::vector<std::function<void()>> m_IconCallbacks;
    bool m_Built = false;
};

} // namespace we::runtime::kindui
