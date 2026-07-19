#include "ViewportEditInternal.h"

#include <algorithm>

namespace we::editor::viewportedit {
namespace detail {

class ViewportSelectionImpl final : public IViewportSelection {
public:
    [[nodiscard]] std::span<const ViewportObjectId> GetSelected() const noexcept override {
        return m_Selected;
    }

    [[nodiscard]] ViewportObjectId GetPrimary() const noexcept override {
        return m_Selected.empty() ? ViewportObjectId{} : m_Selected.front();
    }

    [[nodiscard]] bool IsSelected(ViewportObjectId id) const noexcept override {
        return std::find(m_Selected.begin(), m_Selected.end(), id) != m_Selected.end();
    }

    [[nodiscard]] bool IsEmpty() const noexcept override { return m_Selected.empty(); }
    [[nodiscard]] std::size_t Count() const noexcept override { return m_Selected.size(); }

    void Clear() override {
        if (m_Selected.empty()) {
            return;
        }
        m_Selected.clear();
        Notify();
    }

    void Set(ViewportObjectId id) override {
        if (!id.IsValid()) {
            Clear();
            return;
        }
        if (m_Selected.size() == 1 && m_Selected[0] == id) {
            return;
        }
        m_Selected.clear();
        m_Selected.push_back(id);
        Notify();
    }

    void SetMany(std::span<const ViewportObjectId> ids) override {
        m_Selected.assign(ids.begin(), ids.end());
        Notify();
    }

    void Apply(SelectionOp op, ViewportObjectId id) override {
        if (!id.IsValid() || IsLocked(id)) {
            return;
        }
        switch (op) {
        case SelectionOp::Replace:
            Set(id);
            return;
        case SelectionOp::Add:
            if (!IsSelected(id)) {
                m_Selected.push_back(id);
                Notify();
            }
            return;
        case SelectionOp::Toggle:
            if (IsSelected(id)) {
                m_Selected.erase(
                    std::remove(m_Selected.begin(), m_Selected.end(), id),
                    m_Selected.end());
            } else {
                m_Selected.push_back(id);
            }
            Notify();
            return;
        case SelectionOp::Remove:
            if (IsSelected(id)) {
                m_Selected.erase(
                    std::remove(m_Selected.begin(), m_Selected.end(), id),
                    m_Selected.end());
                Notify();
            }
            return;
        }
    }

    void ApplyMany(SelectionOp op, std::span<const ViewportObjectId> ids) override {
        if (op == SelectionOp::Replace) {
            SetMany(ids);
            return;
        }
        for (const auto id : ids) {
            Apply(op, id);
        }
    }

    [[nodiscard]] ViewportObjectId GetHovered() const noexcept override { return m_Hovered; }
    void SetHovered(ViewportObjectId id) override { m_Hovered = id; }

    void SetLocked(ViewportObjectId id, bool locked) override {
        if (locked) {
            m_Locked.insert(id.value);
        } else {
            m_Locked.erase(id.value);
        }
    }

    void SetHidden(ViewportObjectId id, bool hidden) override {
        if (hidden) {
            m_Hidden.insert(id.value);
        } else {
            m_Hidden.erase(id.value);
        }
    }

    [[nodiscard]] bool IsLocked(ViewportObjectId id) const noexcept override {
        return m_Locked.contains(id.value);
    }

    [[nodiscard]] bool IsHidden(ViewportObjectId id) const noexcept override {
        return m_Hidden.contains(id.value);
    }

    void AddListener(Listener listener) override {
        if (listener) {
            m_Listeners.push_back(std::move(listener));
        }
    }

private:
    void Notify() {
        ViewportEditDiagnostics::Get().OnSelectionChanged(m_Selected.size());
        for (auto& listener : m_Listeners) {
            listener();
        }
    }

    std::vector<ViewportObjectId> m_Selected;
    ViewportObjectId m_Hovered{};
    std::unordered_set<std::uint64_t> m_Locked;
    std::unordered_set<std::uint64_t> m_Hidden;
    std::vector<Listener> m_Listeners;
};

std::unique_ptr<IViewportSelection> CreateSelection() {
    return std::make_unique<ViewportSelectionImpl>();
}

} // namespace detail
} // namespace we::editor::viewportedit
