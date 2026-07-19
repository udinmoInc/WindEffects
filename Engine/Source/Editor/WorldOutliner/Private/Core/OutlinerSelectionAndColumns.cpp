#include "OutlinerInternal.h"

#include "Undo/UndoTypes.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace we::editor::outliner {
namespace detail {
namespace {

std::string ToLower(std::string_view in) {
    std::string out(in);
    for (char& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

class EventRouterImpl final : public IOutlinerEventRouter {
public:
    void Publish(const OutlinerEvent& event) override {
        if (m_Suspended) {
            return;
        }
        for (auto& listener : m_Listeners) {
            listener(event);
        }
    }

    void Subscribe(OutlinerEventListener listener) override {
        if (listener) {
            m_Listeners.push_back(std::move(listener));
        }
    }

    void Suspend(bool suspend) override { m_Suspended = suspend; }
    [[nodiscard]] bool IsSuspended() const noexcept override { return m_Suspended; }

private:
    std::vector<OutlinerEventListener> m_Listeners;
    bool m_Suspended = false;
};

class SelectionImpl final : public IOutlinerSelection {
public:
    explicit SelectionImpl(IOutlinerEventRouter& events)
        : m_Events(events)
    {}

    [[nodiscard]] std::span<const OutlinerNodeId> GetSelected() const noexcept override { return m_Selected; }
    [[nodiscard]] OutlinerNodeId GetPrimary() const noexcept override {
        return m_Selected.empty() ? OutlinerNodeId{} : m_Selected.front();
    }
    [[nodiscard]] bool IsSelected(OutlinerNodeId id) const noexcept override {
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

    void Set(OutlinerNodeId id) override {
        if (!id.IsValid()) {
            Clear();
            return;
        }
        if (m_Selected.size() == 1 && m_Selected[0] == id) {
            return;
        }
        m_Selected = {id};
        Notify();
    }

    void SetMany(std::span<const OutlinerNodeId> ids) override {
        m_Selected.assign(ids.begin(), ids.end());
        Notify();
    }

    void Apply(OutlinerSelectionOp op, OutlinerNodeId id) override {
        if (!id.IsValid()) {
            return;
        }
        switch (op) {
        case OutlinerSelectionOp::Replace:
            Set(id);
            return;
        case OutlinerSelectionOp::Add:
            if (!IsSelected(id)) {
                m_Selected.push_back(id);
                Notify();
            }
            return;
        case OutlinerSelectionOp::Toggle:
            if (IsSelected(id)) {
                m_Selected.erase(std::remove(m_Selected.begin(), m_Selected.end(), id), m_Selected.end());
            } else {
                m_Selected.push_back(id);
            }
            Notify();
            return;
        case OutlinerSelectionOp::Remove:
            if (IsSelected(id)) {
                m_Selected.erase(std::remove(m_Selected.begin(), m_Selected.end(), id), m_Selected.end());
                Notify();
            }
            return;
        }
    }

    [[nodiscard]] std::span<const OutlinerNodeId> GetHistory() const noexcept override { return m_History; }

    void PushHistory() override {
        if (m_Selected.empty()) {
            return;
        }
        m_History.resize(m_HistoryIndex);
        m_History.push_back(GetPrimary());
        if (m_History.size() > 64) {
            m_History.erase(m_History.begin());
        }
        m_HistoryIndex = m_History.size();
    }

    bool NavigateHistoryBack() override {
        if (m_HistoryIndex == 0 || m_History.empty()) {
            return false;
        }
        --m_HistoryIndex;
        Set(m_History[m_HistoryIndex]);
        return true;
    }

    bool NavigateHistoryForward() override {
        if (m_HistoryIndex + 1 >= m_History.size()) {
            return false;
        }
        ++m_HistoryIndex;
        Set(m_History[m_HistoryIndex]);
        return true;
    }

    void SetSilent(std::span<const OutlinerNodeId> ids) {
        m_Selected.assign(ids.begin(), ids.end());
    }

private:
    void Notify() {
        WorldOutlinerDiagnostics::Get().OnSelectionChanged(m_Selected.size());
        OutlinerEvent ev;
        ev.kind = OutlinerEventKind::SelectionChanged;
        ev.nodes = m_Selected;
        ev.primary = GetPrimary();
        m_Events.Publish(ev);
    }

    IOutlinerEventRouter& m_Events;
    std::vector<OutlinerNodeId> m_Selected;
    std::vector<OutlinerNodeId> m_History;
    std::size_t m_HistoryIndex = 0;
};

class SearchImpl final : public IOutlinerSearch {
public:
    void SetQuery(std::string_view query) override {
        m_Query = std::string(query);
        m_QueryLower = ToLower(m_Query);
    }
    [[nodiscard]] std::string_view GetQuery() const noexcept override { return m_Query; }

    [[nodiscard]] bool Matches(const IOutlinerNode& node) const override {
        if (m_QueryLower.empty()) {
            return true;
        }
        const auto name = ToLower(node.GetDisplayName());
        const auto type = ToLower(node.GetTypeName());
        return name.find(m_QueryLower) != std::string::npos || type.find(m_QueryLower) != std::string::npos;
    }

    [[nodiscard]] std::vector<OutlinerNodeId> Query(std::span<const OutlinerNodeId> candidates) const override {
        std::vector<OutlinerNodeId> out;
        (void)candidates;
        return out;
    }

private:
    std::string m_Query;
    std::string m_QueryLower;
};

class SorterImpl final : public IOutlinerSorter {
public:
    explicit SorterImpl(IOutlinerTreeModel& model)
        : m_Model(model)
    {}

    void Sort(std::vector<OutlinerNodeId>& ids, OutlinerSortMode mode) const override {
        std::sort(ids.begin(), ids.end(), [&](OutlinerNodeId a, OutlinerNodeId b) {
            const auto* na = m_Model.GetNode(a);
            const auto* nb = m_Model.GetNode(b);
            if (!na || !nb) {
                return a.value < b.value;
            }
            switch (mode) {
            case OutlinerSortMode::NameDesc:
                return na->GetDisplayName() > nb->GetDisplayName();
            case OutlinerSortMode::TypeAsc:
                return na->GetTypeName() < nb->GetTypeName();
            case OutlinerSortMode::RecentlyModified:
            case OutlinerSortMode::NameAsc:
            default:
                return na->GetDisplayName() < nb->GetDisplayName();
            }
        });
    }

private:
    IOutlinerTreeModel& m_Model;
};

class DefaultFilter final : public IOutlinerFilter {
public:
    [[nodiscard]] bool Passes(const IOutlinerNode& node, const OutlinerFilterState& state) const override {
        WorldOutlinerDiagnostics::Get().OnFilterPass();
        if (!state.showFolders && node.GetKind() == OutlinerNodeKind::Folder) {
            return false;
        }
        if (!state.showActors && node.GetKind() == OutlinerNodeKind::Actor) {
            return false;
        }
        if (!state.showHidden && !node.GetFlags().visible) {
            return false;
        }
        if (!state.showLocked && node.GetFlags().locked) {
            return false;
        }
        if (state.favoritesOnly && !node.GetFlags().favorite) {
            return false;
        }
        return true;
    }
};

class NameColumn final : public IOutlinerColumnProvider {
public:
    [[nodiscard]] OutlinerColumnId GetColumnId() const noexcept override { return OutlinerColumnId::Name; }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return "Name"; }
    [[nodiscard]] std::string GetCellText(const IOutlinerNode& node) const override {
        return std::string(node.GetDisplayName());
    }
    [[nodiscard]] float GetPreferredWidth() const noexcept override { return 220.f; }
};

class TypeColumn final : public IOutlinerColumnProvider {
public:
    [[nodiscard]] OutlinerColumnId GetColumnId() const noexcept override { return OutlinerColumnId::Type; }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return "Type"; }
    [[nodiscard]] std::string GetCellText(const IOutlinerNode& node) const override {
        return std::string(node.GetTypeName());
    }
    [[nodiscard]] float GetPreferredWidth() const noexcept override { return 120.f; }
};

class LayerColumn final : public IOutlinerColumnProvider {
public:
    [[nodiscard]] OutlinerColumnId GetColumnId() const noexcept override { return OutlinerColumnId::Layer; }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return "Layer"; }
    [[nodiscard]] std::string GetCellText(const IOutlinerNode& node) const override {
        return std::string(node.GetLayer());
    }
    [[nodiscard]] float GetPreferredWidth() const noexcept override { return 80.f; }
};

class TagColumn final : public IOutlinerColumnProvider {
public:
    [[nodiscard]] OutlinerColumnId GetColumnId() const noexcept override { return OutlinerColumnId::Tag; }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return "Tags"; }
    [[nodiscard]] std::string GetCellText(const IOutlinerNode& node) const override {
        std::ostringstream oss;
        bool first = true;
        for (const auto& tag : node.GetTags()) {
            if (!first) {
                oss << ", ";
            }
            oss << tag;
            first = false;
        }
        return oss.str();
    }
    [[nodiscard]] float GetPreferredWidth() const noexcept override { return 100.f; }
};

class VisibilityColumn final : public IOutlinerColumnProvider {
public:
    [[nodiscard]] OutlinerColumnId GetColumnId() const noexcept override { return OutlinerColumnId::Visibility; }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return "Visible"; }
    [[nodiscard]] std::string GetCellText(const IOutlinerNode& node) const override {
        return node.GetFlags().visible ? "Yes" : "No";
    }
    [[nodiscard]] float GetPreferredWidth() const noexcept override { return 56.f; }
};

class LockColumn final : public IOutlinerColumnProvider {
public:
    [[nodiscard]] OutlinerColumnId GetColumnId() const noexcept override { return OutlinerColumnId::Lock; }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return "Lock"; }
    [[nodiscard]] std::string GetCellText(const IOutlinerNode& node) const override {
        return node.GetFlags().locked ? "Locked" : "";
    }
    [[nodiscard]] float GetPreferredWidth() const noexcept override { return 56.f; }
};

} // namespace

// Forward decl helpers used by OutlinerImpl in Runtime TU
std::unique_ptr<IOutlinerEventRouter> CreateEventRouter() {
    return std::make_unique<EventRouterImpl>();
}
std::unique_ptr<IOutlinerSelection> CreateSelection(IOutlinerEventRouter& events) {
    return std::make_unique<SelectionImpl>(events);
}
std::unique_ptr<IOutlinerSearch> CreateSearch() {
    return std::make_unique<SearchImpl>();
}
std::unique_ptr<IOutlinerFilter> CreateDefaultFilter() {
    return std::make_unique<DefaultFilter>();
}
std::unique_ptr<IOutlinerColumnProvider> CreateNameColumn() { return std::make_unique<NameColumn>(); }
std::unique_ptr<IOutlinerColumnProvider> CreateTypeColumn() { return std::make_unique<TypeColumn>(); }
std::unique_ptr<IOutlinerColumnProvider> CreateLayerColumn() { return std::make_unique<LayerColumn>(); }
std::unique_ptr<IOutlinerColumnProvider> CreateTagColumn() { return std::make_unique<TagColumn>(); }
std::unique_ptr<IOutlinerColumnProvider> CreateVisibilityColumn() { return std::make_unique<VisibilityColumn>(); }
std::unique_ptr<IOutlinerColumnProvider> CreateLockColumn() { return std::make_unique<LockColumn>(); }

SelectionImpl* AsSelectionImpl(IOutlinerSelection& sel) {
    return dynamic_cast<SelectionImpl*>(&sel);
}

} // namespace detail
} // namespace we::editor::outliner
