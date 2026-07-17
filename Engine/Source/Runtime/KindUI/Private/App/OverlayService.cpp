#include "KindUI/App/OverlayService.h"

namespace we::runtime::kindui {

OverlayService::OverlayService(std::shared_ptr<ModalHost> host)
    : m_Host(std::move(host))
    , Visible(false) {
    if (m_Host) {
        m_Host->SetVisible(false);
        m_Host->SetDismissOnScrim(false);
    }
}

void OverlayService::ShowBusy(const std::string message) {
    m_Kind = OverlayKind::Busy;
    m_Message = message;
    Visible = true;
    if (m_Host) {
        m_Host->SetVisible(true);
    }
}

void OverlayService::ShowLoading(const std::string message) {
    m_Kind = OverlayKind::Loading;
    m_Message = message;
    Visible = true;
    if (m_Host) {
        m_Host->SetVisible(true);
    }
}

void OverlayService::ShowBlocking(const std::string message) {
    m_Kind = OverlayKind::Blocking;
    m_Message = message;
    Visible = true;
    if (m_Host) {
        m_Host->SetVisible(true);
    }
}

void OverlayService::Hide() {
    m_Kind = OverlayKind::None;
    m_Message.clear();
    Visible = false;
    if (m_Host) {
        m_Host->SetContent(nullptr);
        m_Host->SetVisible(false);
    }
}

} // namespace we::runtime::kindui
