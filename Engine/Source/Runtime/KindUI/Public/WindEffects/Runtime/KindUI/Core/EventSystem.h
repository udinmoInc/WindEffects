#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Input/InputEvents.h"

#include <memory>
#include <vector>

namespace WindEffects::Editor::UI {

class Widget;
class OverlayHost;

class UI_API EventSystem {
public:
    EventSystem() = default;
    ~EventSystem();

    void SetRootWidget(const std::shared_ptr<Widget>& root) { m_Root = root; }
    std::shared_ptr<Widget> GetRootWidget() const { return m_Root; }

    void ProcessMouseEvent(const MouseEvent& event);
    void ProcessKeyEvent(const KeyEvent& event);

    std::shared_ptr<Widget> GetFocusedWidget() const { return m_FocusedWidget.lock(); }
    std::shared_ptr<Widget> GetHoveredWidget() const { return m_HoveredWidget.lock(); }
    std::shared_ptr<Widget> GetCapturedWidget() const { return m_CapturedWidget.lock(); }

    void SetFocusedWidget(const std::shared_ptr<Widget>& widget);
    void SetCapturedWidget(const std::shared_ptr<Widget>& widget) { m_CapturedWidget = widget; }
    void SetSuppressSystemCursor(bool suppress) { m_SuppressSystemCursor = suppress; }

    void SetPopupHost(OverlayHost* popupHost) { m_PopupHost = popupHost; }

    /// Move keyboard focus to the next/previous focusable widget in tree order.
    void FocusNext(bool reverse = false);

    static std::shared_ptr<Widget> HitTest(const std::shared_ptr<Widget>& root, const Point& pos);

private:
    void UpdateCursorForWidget(const std::shared_ptr<Widget>& widget, const Point& position);
    void CollectFocusable(const std::shared_ptr<Widget>& node, std::vector<std::shared_ptr<Widget>>& out) const;

#pragma warning(push)
#pragma warning(disable: 4251)
    std::shared_ptr<Widget> m_Root;
    std::weak_ptr<Widget> m_FocusedWidget;
    std::weak_ptr<Widget> m_HoveredWidget;
    std::weak_ptr<Widget> m_CapturedWidget;
#pragma warning(pop)
    OverlayHost* m_PopupHost = nullptr;
    bool m_UsingPointerCursor = false;
    bool m_SuppressSystemCursor = false;
};

} // namespace WindEffects::Editor::UI
