#pragma once

#include "KindUI/App/ViewHost.h"
#include "KindUI/Declarative/Element.h"
#include "KindUI/Export.h"
#include "KindUI/Layout/IPopupHost.h"

#include <functional>
#include <memory>

namespace we::runtime::kindui {

/// Positioned popup presenter for context menus, dropdowns, and floating panels.
class KINDUI_API PopupService {
public:
    PopupService(IPopupHost* host, std::shared_ptr<IWidgetContext> context);

    void ShowMenu(Element menu, Point position);
    void ShowMenuAt(Element menu, const Widget& anchor);
    void ShowDropdown(Element menu, Point position);
    void DismissTop();
    void DismissAll();

    [[nodiscard]] bool HasOpenPopups() const;

private:
    std::shared_ptr<Widget> BuildMenu(const Element& menu);

    IPopupHost* m_Host = nullptr;
    std::shared_ptr<IWidgetContext> m_Context;
    std::unique_ptr<ViewHost> m_ViewHost;
};

} // namespace we::runtime::kindui
