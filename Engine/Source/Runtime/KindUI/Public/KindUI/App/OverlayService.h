#pragma once

#include "KindUI/Core/Observable.h"
#include "KindUI/Export.h"
#include "KindUI/Widgets/ModalHost.h"

#include <memory>
#include <string>

namespace we::runtime::kindui {

enum class OverlayKind {
    None,
    Busy,
    Loading,
    Blocking,
};

/// Blocking/busy overlay presenter.
class KINDUI_API OverlayService {
public:
    explicit OverlayService(std::shared_ptr<ModalHost> host = MakeModalHost());

    void ShowBusy(std::string message = "Working...");
    void ShowLoading(std::string message = "Loading...");
    void ShowBlocking(std::string message);
    void Hide();

    [[nodiscard]] OverlayKind Kind() const { return m_Kind; }
    [[nodiscard]] const std::string& Message() const { return m_Message; }
    [[nodiscard]] std::shared_ptr<ModalHost> Host() const { return m_Host; }

    Observable<bool> Visible;

private:
    std::shared_ptr<ModalHost> m_Host;
    OverlayKind m_Kind = OverlayKind::None;
    std::string m_Message;
};

} // namespace we::runtime::kindui
