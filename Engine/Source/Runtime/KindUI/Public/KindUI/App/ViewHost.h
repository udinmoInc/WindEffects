#pragma once

#include "KindUI/Export.h"
#include "KindUI/Declarative/Element.h"
#include "KindUI/Core/IApplicationContext.h"
#include "KindUI/Core/Observable.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/WidgetContext.h"
#include "KindUI/Layout/IPopupHost.h"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace we::runtime::kindui {

class ViewBuilder;

/// Retained view host with incremental reconciliation and view factories.
class KINDUI_API ViewHost {
public:
    explicit ViewHost(IApplicationContext& app, IPopupHost* popupHost = nullptr);
    explicit ViewHost(std::shared_ptr<IWidgetContext> context);
    ~ViewHost();

    void SetView(const Element& view);
    void Reconcile(const Element& view);
    void Clear();

    /// Describe the view from application state; call Refresh() after state changes.
    void SetViewFactory(std::function<Element()> factory);
    void Refresh();

    /// Wire observable state to automatic Refresh(). Call after SetViewFactory.
    template <typename T>
    void Observe(Observable<T>& source) {
        (void)source.Subscribe([this](const T&) { Refresh(); });
    }

    template <typename T>
    void ObserveList(ObservableList<T>& list) {
        (void)list.Subscribe([this] { Refresh(); });
    }

    [[nodiscard]] std::shared_ptr<Widget> GetRoot() const { return m_Root; }
    [[nodiscard]] IWidgetContext& GetContext() const { return *m_Context; }
    [[nodiscard]] std::shared_ptr<IWidgetContext> SharedContext() const { return m_Context; }
    [[nodiscard]] bool HasView() const { return m_Root != nullptr; }
    [[nodiscard]] const std::optional<Element>& CurrentDescription() const { return m_Description; }

    void SetPopupHost(IPopupHost* popupHost);

private:
    std::shared_ptr<WidgetContext> m_Context;
    std::unique_ptr<ViewBuilder> m_Builder;
    std::shared_ptr<Widget> m_Root;
    std::optional<Element> m_Description;
    std::function<Element()> m_ViewFactory;
};

} // namespace we::runtime::kindui
