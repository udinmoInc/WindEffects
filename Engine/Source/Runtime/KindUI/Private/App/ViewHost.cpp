// ==============================================================================
// WindEffects — KindUI — ViewHost
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Host/ViewHost.h"

#include "KindUI/Host/DialogService.h"
#include "App/PopupService.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Compose/ViewBuilder.h"

namespace we::runtime::kindui {
namespace {

void MarkViewDirty(const std::shared_ptr<Widget>& root) {
    UIRepaintGate::Request();
    if (root) {
        root->InvalidateLayout();
    }
}

} // namespace

ApplicationServices::ApplicationServices() = default;
ApplicationServices::~ApplicationServices() = default;
ApplicationServices::ApplicationServices(ApplicationServices&&) noexcept = default;
ApplicationServices& ApplicationServices::operator=(ApplicationServices&&) noexcept = default;

void ApplicationServices::Initialize(std::shared_ptr<IWidgetContext> widgetContext, IPopupHost* host) {
    context = std::move(widgetContext);
    popupHost = host;
    dialogs = std::make_unique<DialogService>(context);
    popups = std::make_unique<PopupService>(host, context);
}

ViewHost::ViewHost(IApplicationContext& app, IPopupHost* popupHost)
    : m_Context(std::make_shared<WidgetContext>(app, popupHost))
    , m_Builder(std::make_unique<ViewBuilder>(m_Context)) {}

ViewHost::ViewHost(std::shared_ptr<IWidgetContext> context)
    : m_Builder(std::make_unique<ViewBuilder>(context)) {
    m_Context = std::dynamic_pointer_cast<WidgetContext>(context);
}

ViewHost::~ViewHost() = default;

void ViewHost::SetView(const Element& view) {
    m_Description = view;
    m_Root = m_Builder->Build(view);
    MarkViewDirty(m_Root);
}

void ViewHost::Reconcile(const Element& view) {
    m_Description = view;
    if (!m_Root) {
        m_Root = m_Builder->Build(view);
        MarkViewDirty(m_Root);
        return;
    }
    m_Builder->Reconcile(*m_Root, view);
    MarkViewDirty(m_Root);
}

void ViewHost::Clear() {
    m_Root.reset();
    m_Description.reset();
    UIRepaintGate::Request();
}

void ViewHost::SetViewFactory(std::function<Element()> factory) {
    m_ViewFactory = std::move(factory);
}

void ViewHost::Refresh() {
    if (!m_ViewFactory) {
        return;
    }
    const Element view = m_ViewFactory();
    if (m_Root) {
        Reconcile(view);
    } else {
        SetView(view);
    }
}

void ViewHost::SetPopupHost(IPopupHost* popupHost) {
    if (m_Context) {
        m_Context->SetPopupHost(popupHost);
    }
}

} // namespace we::runtime::kindui
 
