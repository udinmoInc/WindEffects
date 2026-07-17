#include "KindUI/App/DialogService.h"

#include "KindUI/Declarative/UI.h"

namespace we::runtime::kindui {

DialogService::DialogService(std::shared_ptr<IWidgetContext> context)
    : m_Context(std::move(context))
    , m_ModalHost(MakeModalHost()) {
    m_ModalHost->SetVisible(false);
    m_ModalHost->SetOnScrimClicked([this] {
        if (m_Spec.dismissOnScrim) {
            Dismiss();
        }
    });
}

void DialogService::Show(ViewFactory factory, DialogSpec spec) {
    if (!m_Context || !factory) {
        return;
    }
    m_Factory = std::move(factory);
    m_Spec = std::move(spec);
    m_ActiveId = m_Spec.id.empty() ? std::optional<std::string>{} : std::optional<std::string>{ m_Spec.id };

    if (!m_ViewHost) {
        m_ViewHost = std::make_unique<ViewHost>(m_Context);
    }
    m_ViewHost->SetViewFactory([this] { return m_Factory(); });
    m_ViewHost->Refresh();

    m_ModalHost->SetDialogWidth(m_Spec.width);
    m_ModalHost->SetDialogHeight(m_Spec.height);
    m_ModalHost->SetDismissOnScrim(m_Spec.dismissOnScrim);
    m_ModalHost->SetContent(m_ViewHost->GetRoot());
    m_ModalHost->SetVisible(true);
    m_Open = true;
}

void DialogService::Refresh() {
    if (!m_Open || !m_ViewHost) {
        return;
    }
    m_ViewHost->Refresh();
    m_ModalHost->SetContent(m_ViewHost->GetRoot());
    m_ModalHost->InvalidateLayout();
}

void DialogService::Dismiss(const std::string& id) {
    if (!m_Open) {
        return;
    }
    if (!id.empty() && m_ActiveId && *m_ActiveId != id) {
        return;
    }
    m_Open = false;
    m_ActiveId.reset();
    m_Factory = {};
    m_ModalHost->SetContent(nullptr);
    m_ModalHost->SetVisible(false);
}

void DialogService::DismissAll() {
    Dismiss();
}

bool DialogService::IsOpen(const std::string& id) const {
    return m_Open && m_ActiveId && *m_ActiveId == id;
}

void DialogService::ShowMessage(
    std::string title,
    std::string message,
    DialogKind kind,
    std::function<void()> onDismiss) {
    DialogSpec spec;
    spec.id = "message";
    spec.kind = kind;
    spec.width = 460.0f;
    Show(
        [title = std::move(title), message = std::move(message), onDismiss = std::move(onDismiss), this]() {
            return UI::Column({
                UI::Section(title, message),
                UI::Row({
                    UI::Spacer(),
                    UI::OnClick(UI::Button("OK", WidgetVariant::Primary), [this, onDismiss] {
                        if (onDismiss) {
                            onDismiss();
                        }
                        Dismiss("message");
                    }),
                }),
            });
        },
        spec);
}

void DialogService::ShowConfirmation(
    std::string title,
    std::string message,
    std::function<void(bool confirmed)> onResult,
    std::string confirmLabel,
    std::string cancelLabel) {
    DialogSpec spec;
    spec.id = "confirm";
    spec.kind = DialogKind::Confirmation;
    spec.width = 480.0f;
    Show(
        [title = std::move(title),
         message = std::move(message),
         onResult = std::move(onResult),
         confirmLabel = std::move(confirmLabel),
         cancelLabel = std::move(cancelLabel),
         this]() {
            return UI::Column({
                UI::Section(title, message),
                UI::Row({
                    UI::Spacer(),
                    UI::OnClick(
                        UI::Button(cancelLabel, WidgetVariant::Secondary),
                        [this, onResult] {
                            if (onResult) {
                                onResult(false);
                            }
                            Dismiss("confirm");
                        }),
                    UI::OnClick(
                        UI::Button(confirmLabel, WidgetVariant::Primary),
                        [this, onResult] {
                            if (onResult) {
                                onResult(true);
                            }
                            Dismiss("confirm");
                        }),
                }),
            });
        },
        spec);
}

} // namespace we::runtime::kindui
