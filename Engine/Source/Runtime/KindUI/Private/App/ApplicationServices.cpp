#include "KindUI/App/ApplicationServices.h"

namespace we::runtime::kindui {

void ApplicationServices::Initialize(std::shared_ptr<IWidgetContext> widgetContext, IPopupHost* host) {
    context = std::move(widgetContext);
    popupHost = host;
    dialogs = std::make_unique<DialogService>(context);
    popups = std::make_unique<PopupService>(host, context);
    overlay = std::make_unique<OverlayService>();
}

} // namespace we::runtime::kindui
