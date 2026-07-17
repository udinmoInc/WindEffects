#include "KindUI/App/PopupService.h"

#include "KindUI/Declarative/ViewBuilder.h"

namespace we::runtime::kindui {

PopupService::PopupService(IPopupHost* host, std::shared_ptr<IWidgetContext> context)
    : m_Host(host)
    , m_Context(std::move(context)) {}

void PopupService::ShowMenu(Element menu, Point position) {
    if (!m_Host) {
        return;
    }
    if (auto popup = BuildMenu(std::move(menu))) {
        m_Host->ShowPopup(popup, position);
    }
}

void PopupService::ShowMenuAt(Element menu, const Widget& anchor) {
    const Rect geom = anchor.GetGeometry();
    ShowMenu(std::move(menu), Point{ geom.x, geom.y + geom.height });
}

void PopupService::ShowDropdown(Element menu, Point position) {
    ShowMenu(std::move(menu), position);
}

void PopupService::DismissTop() {
    if (m_Host) {
        m_Host->CloseTopPopup();
    }
}

void PopupService::DismissAll() {
    if (m_Host) {
        m_Host->CloseAllPopups();
    }
}

bool PopupService::HasOpenPopups() const {
    return m_Host && m_Host->HasOpenPopups();
}

std::shared_ptr<Widget> PopupService::BuildMenu(const Element& menu) {
    if (!m_Context) {
        return nullptr;
    }
    ViewBuilder builder(m_Context);
    return builder.Build(menu);
}

} // namespace we::runtime::kindui
