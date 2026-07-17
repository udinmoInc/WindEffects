#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Core/Widget.h"
#include "WindEffects/Runtime/UI/Core/PaintContext.h"
#include "WindEffects/Runtime/UI/Core/Observable.h"

#include <functional>
#include <vector>

namespace WindEffects::Editor::UI {

/// Virtualized list — only visible rows are measured/arranged/painted.
class UI_API VirtualList : public Widget {
public:
    using ItemFactory = std::function<std::shared_ptr<Widget>(size_t index)>;

    VirtualList() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseWheel(const MouseEvent& event) override;

    void SetItemCount(size_t count);
    void SetItemHeight(float height);
    void SetItemFactory(ItemFactory factory);
    void SetScrollOffset(float offset);
    [[nodiscard]] float GetScrollOffset() const { return m_ScrollOffset; }

    template <typename T>
    void BindItems(ObservableList<T>& list, std::function<std::shared_ptr<Widget>(const T&, size_t)> factory) {
        m_ItemFactory = [factory, &list](size_t index) -> std::shared_ptr<Widget> {
            const auto& items = list.Items();
            if (index >= items.size()) return nullptr;
            return factory(items[index], index);
        };
        SetItemCount(list.Items().size());
        list.Subscribe([this, &list]() {
            SetItemCount(list.Items().size());
            m_Cache.clear();
            InvalidateLayout();
        });
    }

private:
    void RebuildVisible();

    size_t m_ItemCount = 0;
    float m_ItemHeight = 28.0f;
    float m_ScrollOffset = 0.0f;
    ItemFactory m_ItemFactory;
    std::vector<std::shared_ptr<Widget>> m_Cache;
    size_t m_FirstVisible = 0;
    size_t m_VisibleCount = 0;
};

[[nodiscard]] UI_API std::shared_ptr<VirtualList> MakeVirtualList();

} // namespace WindEffects::Editor::UI
