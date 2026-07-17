#pragma once

#include "KindUI/App/ViewHost.h"
#include "KindUI/Core/Observable.h"
#include "KindUI/Declarative/Element.h"
#include "KindUI/Export.h"
#include "KindUI/Widgets/ModalHost.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace we::runtime::kindui {

enum class DialogKind {
    Modal,
    Modeless,
    Confirmation,
    Message,
    Input,
    Wizard,
    Progress,
    Error,
};

struct DialogSpec {
    std::string id;
    DialogKind kind = DialogKind::Modal;
    float width = 520.0f;
    float height = 0.0f;
    bool dismissOnScrim = true;
};

/// Declarative dialog presenter — applications describe dialogs; KindUI builds and hosts them.
class KINDUI_API DialogService {
public:
    using ViewFactory = std::function<Element()>;

    explicit DialogService(std::shared_ptr<IWidgetContext> context);

    void Show(ViewFactory factory, DialogSpec spec = {});
    void Refresh();
    void Dismiss(const std::string& id = {});
    void DismissAll();

    [[nodiscard]] bool IsOpen() const { return m_Open; }
    [[nodiscard]] bool IsOpen(const std::string& id) const;
    [[nodiscard]] const std::optional<std::string>& ActiveId() const { return m_ActiveId; }
    [[nodiscard]] std::shared_ptr<ModalHost> Overlay() const { return m_ModalHost; }

    template <typename T>
    void Observe(Observable<T>& source) {
        (void)source.Subscribe([this](const T&) { Refresh(); });
    }

    template <typename T>
    void ObserveList(ObservableList<T>& list) {
        (void)list.Subscribe([this] { Refresh(); });
    }

    void ShowMessage(
        std::string title,
        std::string message,
        DialogKind kind = DialogKind::Message,
        std::function<void()> onDismiss = {});

    void ShowConfirmation(
        std::string title,
        std::string message,
        std::function<void(bool confirmed)> onResult,
        std::string confirmLabel = "Confirm",
        std::string cancelLabel = "Cancel");

private:
    std::shared_ptr<IWidgetContext> m_Context;
    std::unique_ptr<ViewHost> m_ViewHost;
    std::shared_ptr<ModalHost> m_ModalHost;
    ViewFactory m_Factory;
    DialogSpec m_Spec;
    bool m_Open = false;
    std::optional<std::string> m_ActiveId;
};

} // namespace we::runtime::kindui
