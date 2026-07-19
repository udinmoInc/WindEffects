#include "OutlinerInternal.h"

#include "ViewportEdit/IViewportEditor.h"
#include "ViewportEdit/IViewportSelection.h"
#include "ViewportEdit/ViewportEditSession.h"

#include <utility>

namespace we::editor::outliner {
namespace detail {

class WorldOutlinerImpl final : public IWorldOutliner {
public:
    explicit WorldOutlinerImpl(WorldOutlinerDependencies deps)
        : m_Deps(std::move(deps))
    {
        m_Events = CreateEventRouter();
        m_Selection = CreateSelection(*m_Events);
        m_Search = CreateSearch();
        m_Model = CreateTreeModel();
        m_Folders = CreateFolderProvider(
            m_Deps.scene,
            *m_Events,
            m_Deps.undo ? &m_Deps.undo->Manager() : nullptr);
        m_Commands = CreateCommandRouter(m_Deps, *m_Selection, *m_Folders, *m_Events, *m_Model);
        m_DragDrop = CreateDragDrop(*m_Commands);

        RegisterFilter(CreateDefaultFilter());
        RegisterColumn(CreateNameColumn());
        RegisterColumn(CreateTypeColumn());
        RegisterColumn(CreateLayerColumn());
        RegisterColumn(CreateTagColumn());
        RegisterColumn(CreateVisibilityColumn());
        RegisterColumn(CreateLockColumn());

        if (m_Deps.scene) {
            RegisterDataProvider(CreateSceneDataProvider(m_Deps.scene));
        }
        if (m_Deps.world) {
            RegisterDataProvider(CreateWorldDataProvider(m_Deps.world));
        }

        // Selection event bus → Scene + ViewportEdit (no ad-hoc host wiring).
        m_Events->Subscribe([this](const OutlinerEvent& event) {
            if (event.kind == OutlinerEventKind::SelectionChanged) {
                SyncSelectionToScene();
                SyncSelectionToViewport();
                m_Selection->PushHistory();
            } else if (
                event.kind == OutlinerEventKind::HierarchyChanged
                || event.kind == OutlinerEventKind::NodeRenamed
                || event.kind == OutlinerEventKind::NodeReparented
                || event.kind == OutlinerEventKind::NodeDeleted)
            {
                RequestRebuild();
            }
        });

        RebuildProvidersAndModel();
    }

    [[nodiscard]] IOutlinerTreeModel& Model() noexcept override { return *m_Model; }
    [[nodiscard]] IOutlinerSelection& Selection() noexcept override { return *m_Selection; }
    [[nodiscard]] IOutlinerSearch& Search() noexcept override { return *m_Search; }
    [[nodiscard]] IOutlinerCommandRouter& Commands() noexcept override { return *m_Commands; }
    [[nodiscard]] IOutlinerEventRouter& Events() noexcept override { return *m_Events; }
    [[nodiscard]] IOutlinerDragDropHandler& DragDrop() noexcept override { return *m_DragDrop; }
    [[nodiscard]] IOutlinerFolderProvider& Folders() noexcept override { return *m_Folders; }

    void RegisterDataProvider(std::unique_ptr<IOutlinerDataProvider> provider) override {
        if (!provider) {
            return;
        }
        m_Providers.push_back(std::move(provider));
        RefreshModelWiring();
        RequestRebuild();
    }

    void RegisterColumn(std::unique_ptr<IOutlinerColumnProvider> column) override {
        if (column) {
            m_Columns.push_back(std::move(column));
        }
    }

    void RegisterDecorator(std::unique_ptr<IOutlinerDecorator> decorator) override {
        if (decorator) {
            m_Decorators.push_back(std::move(decorator));
        }
    }

    void RegisterContextMenu(std::unique_ptr<IOutlinerContextMenuProvider> provider) override {
        if (provider) {
            m_ContextMenus.push_back(std::move(provider));
        }
    }

    void RegisterFilter(std::unique_ptr<IOutlinerFilter> filter) override {
        if (!filter) {
            return;
        }
        m_Filters.push_back(std::move(filter));
        RefreshModelWiring();
    }

    void BindTreeView(const std::shared_ptr<::we::editor::contentbrowser::TreeView>& tree) override {
        m_Tree = tree;
        if (!tree) {
            return;
        }

        tree->SetOnSelectionChanged([this](const std::vector<std::string>& ids) {
            if (m_ApplyingExternalSelection) {
                return;
            }
            std::vector<OutlinerNodeId> nodeIds;
            nodeIds.reserve(ids.size());
            for (const auto& id : ids) {
                const auto value = static_cast<std::uint64_t>(std::strtoull(id.c_str(), nullptr, 10));
                if (value != 0) {
                    nodeIds.push_back(OutlinerNodeId{value});
                }
            }
            if (nodeIds.empty()) {
                m_Selection->Clear();
            } else {
                m_Selection->SetMany(nodeIds);
            }
        });

        tree->SetOnRenameCommitted([this](const std::string& id, const std::string& newLabel) {
            const auto value = static_cast<std::uint64_t>(std::strtoull(id.c_str(), nullptr, 10));
            if (value != 0) {
                (void)m_Commands->Rename(OutlinerNodeId{value}, newLabel);
            }
        });

        tree->SetOnReparentRequested([this](const std::string& childId, const std::string& newParentId) {
            const auto child = static_cast<std::uint64_t>(std::strtoull(childId.c_str(), nullptr, 10));
            const auto parent = static_cast<std::uint64_t>(std::strtoull(newParentId.c_str(), nullptr, 10));
            if (child != 0) {
                (void)m_DragDrop->Drop(OutlinerNodeId{child}, OutlinerNodeId{parent});
            }
        });

        tree->SetOnVisibilityToggled([this](const std::string& id, bool visible) {
            const auto value = static_cast<std::uint64_t>(std::strtoull(id.c_str(), nullptr, 10));
            if (value != 0) {
                (void)m_Commands->SetVisible(OutlinerNodeId{value}, visible);
            }
        });

        tree->SetOnLockToggled([this](const std::string& id, bool locked) {
            const auto value = static_cast<std::uint64_t>(std::strtoull(id.c_str(), nullptr, 10));
            if (value != 0) {
                (void)m_Commands->SetLocked(OutlinerNodeId{value}, locked);
            }
        });

        tree->SetOnItemDoubleClicked([this](const std::string& id) {
            const auto value = static_cast<std::uint64_t>(std::strtoull(id.c_str(), nullptr, 10));
            if (value != 0) {
                m_Selection->Set(OutlinerNodeId{value});
                (void)m_Commands->FocusSelected();
            }
        });

        PushModelToTreeView();
    }

    void RequestRebuild() override {
        m_Model->MarkDirty();
    }

    void Tick(float) override {
        SyncSelectionFromViewport();
        SyncSelectionFromScene();
        if (m_Model->IsDirty()) {
            RebuildProvidersAndModel();
            PushModelToTreeView();
        }
    }

    void SetFilterState(const OutlinerFilterState& state) override {
        m_Model->SetFilterState(state);
        if (auto tree = m_Tree.lock()) {
            tree->SetSearchQuery(state.searchQuery);
            ::we::editor::contentbrowser::TreeView::FilterOptions opts;
            opts.showFolders = state.showFolders;
            opts.showActors = state.showActors;
            opts.showComponents = state.showComponents;
            opts.showHidden = state.showHidden;
            opts.showLocked = state.showLocked;
            opts.showEmptyFolders = state.showEmptyFolders;
            opts.showFavorites = state.favoritesOnly;
            opts.sortOrder = static_cast<int>(state.sortMode);
            tree->SetFilterOptions(opts);
        }
        OutlinerEvent ev;
        ev.kind = OutlinerEventKind::FilterChanged;
        m_Events->Publish(ev);
    }

    [[nodiscard]] const OutlinerFilterState& GetFilterState() const noexcept override {
        return m_Model->GetFilterState();
    }

    void SaveFilterPreset(std::string_view name) override {
        OutlinerFilterPreset preset;
        preset.name = std::string(name);
        preset.state = m_Model->GetFilterState();
        m_Presets[preset.name] = preset;
    }

    [[nodiscard]] bool LoadFilterPreset(std::string_view name) override {
        auto it = m_Presets.find(std::string(name));
        if (it == m_Presets.end()) {
            return false;
        }
        SetFilterState(it->second.state);
        return true;
    }

    [[nodiscard]] std::vector<OutlinerFilterPreset> ListFilterPresets() const override {
        std::vector<OutlinerFilterPreset> list;
        list.reserve(m_Presets.size());
        for (const auto& [_, preset] : m_Presets) {
            list.push_back(preset);
        }
        return list;
    }

    void SyncSelectionToScene() override {
        if (!m_Deps.scene) {
            return;
        }
        const auto primary = m_Selection->GetPrimary();
        if (!primary.IsValid()) {
            m_Deps.scene->SetSelectedEntityIndex(-1);
            return;
        }
        m_Deps.scene->SetSelectedEntityId(primary.value);
    }

    void SyncSelectionFromScene() override {
        if (!m_Deps.scene) {
            return;
        }
        const auto id = m_Deps.scene->GetSelectedEntityId();
        if (id == 0) {
            if (!m_Selection->IsEmpty()) {
                m_ApplyingExternalSelection = true;
                m_Events->Suspend(true);
                m_Selection->Clear();
                m_Events->Suspend(false);
                m_ApplyingExternalSelection = false;
            }
            return;
        }
        const OutlinerNodeId node{id};
        if (m_Selection->GetPrimary() != node) {
            m_ApplyingExternalSelection = true;
            m_Events->Suspend(true);
            m_Selection->Set(node);
            m_Events->Suspend(false);
            ApplySelectionToTreeView();
            m_ApplyingExternalSelection = false;
        }
    }

    void SyncSelectionToViewport() override {
        auto* viewport = m_Deps.viewportEdit;
        if (!viewport) {
            if (auto* session = viewportedit::ViewportEditSession::Editor()) {
                viewport = session;
            }
        }
        if (!viewport) {
            return;
        }
        const auto primary = m_Selection->GetPrimary();
        if (!primary.IsValid()) {
            viewport->Selection().Clear();
            return;
        }
        std::vector<viewportedit::ViewportObjectId> ids;
        for (const auto id : m_Selection->GetSelected()) {
            ids.push_back(viewportedit::ViewportObjectId{id.value});
        }
        viewport->Selection().SetMany(ids);
    }

    void SyncSelectionFromViewport() override {
        auto* viewport = m_Deps.viewportEdit;
        if (!viewport) {
            if (auto* session = viewportedit::ViewportEditSession::Editor()) {
                viewport = session;
            }
        }
        if (!viewport) {
            return;
        }
        const auto primary = viewport->Selection().GetPrimary();
        if (!primary.IsValid()) {
            return;
        }
        const OutlinerNodeId node{primary.value};
        if (m_Selection->GetPrimary() != node) {
            m_ApplyingExternalSelection = true;
            m_Events->Suspend(true);
            std::vector<OutlinerNodeId> ids;
            for (const auto id : viewport->Selection().GetSelected()) {
                ids.push_back(OutlinerNodeId{id.value});
            }
            m_Selection->SetMany(ids);
            m_Events->Suspend(false);
            // Publish once so Scene/Details update without feedback loops.
            OutlinerEvent ev;
            ev.kind = OutlinerEventKind::SelectionChanged;
            ev.nodes = ids;
            ev.primary = node;
            m_Events->Suspend(false);
            SyncSelectionToScene();
            ApplySelectionToTreeView();
            m_ApplyingExternalSelection = false;
            (void)ev;
        }
    }

private:
    void RefreshModelWiring() {
        std::vector<IOutlinerDataProvider*> providers;
        providers.reserve(m_Providers.size());
        for (auto& p : m_Providers) {
            providers.push_back(p.get());
        }
        TreeModelSetProviders(*m_Model, std::move(providers));

        std::vector<IOutlinerFilter*> filters;
        filters.reserve(m_Filters.size());
        for (auto& f : m_Filters) {
            filters.push_back(f.get());
        }
        TreeModelSetFilters(*m_Model, std::move(filters));
        TreeModelSetSearch(*m_Model, m_Search.get());
    }

    void RebuildProvidersAndModel() {
        RefreshModelWiring();
        m_Model->Rebuild();
        OutlinerEvent ev;
        ev.kind = OutlinerEventKind::ModelRebuilt;
        m_Events->Publish(ev);
    }

    void PushModelToTreeView() {
        auto tree = m_Tree.lock();
        if (!tree) {
            return;
        }

        auto root = std::make_shared<::we::editor::contentbrowser::TreeNode>();
        root->id = "root";
        root->expanded = true;

        std::unordered_map<std::uint64_t, std::shared_ptr<::we::editor::contentbrowser::TreeNode>> byId;
        std::unordered_set<std::uint64_t> visibleSet;
        for (const auto id : m_Model->GetVisibleRows()) {
            visibleSet.insert(id.value);
            if (const auto* node = m_Model->GetNode(id)) {
                auto tn = std::make_shared<::we::editor::contentbrowser::TreeNode>();
                tn->id = std::to_string(id.value);
                tn->label = std::string(node->GetDisplayName());
                tn->iconName = std::string(node->GetIconName());
                tn->expanded = m_Model->IsExpanded(id);
                tn->visible = node->GetFlags().visible;
                tn->locked = node->GetFlags().locked;
                tn->userData = reinterpret_cast<void*>(id.value);
                byId[id.value] = tn;
            }
        }

        // Build hierarchy among visible nodes; attach orphans to root.
        for (const auto& [id, tn] : byId) {
            const auto* node = m_Model->GetNode(OutlinerNodeId{id});
            if (!node) {
                continue;
            }
            const auto parentId = node->GetParentId().value;
            if (parentId != 0 && byId.contains(parentId)) {
                byId[parentId]->children.push_back(tn);
            } else {
                root->children.push_back(tn);
            }
        }

        // Preserve expansion for non-visible ancestors when search filters.
        (void)visibleSet;
        tree->SetRoot(root);
        ApplySelectionToTreeView();
    }

    void ApplySelectionToTreeView() {
        auto tree = m_Tree.lock();
        if (!tree) {
            return;
        }
        std::vector<std::string> ids;
        for (const auto id : m_Selection->GetSelected()) {
            ids.push_back(std::to_string(id.value));
        }
        m_ApplyingExternalSelection = true;
        if (ids.empty()) {
            tree->SetSelectedId("");
        } else {
            tree->SetSelectedIds(ids);
        }
        m_ApplyingExternalSelection = false;
    }

    WorldOutlinerDependencies m_Deps;
    std::unique_ptr<IOutlinerEventRouter> m_Events;
    std::unique_ptr<IOutlinerSelection> m_Selection;
    std::unique_ptr<IOutlinerSearch> m_Search;
    std::unique_ptr<IOutlinerTreeModel> m_Model;
    std::unique_ptr<IOutlinerFolderProvider> m_Folders;
    std::unique_ptr<IOutlinerCommandRouter> m_Commands;
    std::unique_ptr<IOutlinerDragDropHandler> m_DragDrop;

    std::vector<std::unique_ptr<IOutlinerDataProvider>> m_Providers;
    std::vector<std::unique_ptr<IOutlinerFilter>> m_Filters;
    std::vector<std::unique_ptr<IOutlinerColumnProvider>> m_Columns;
    std::vector<std::unique_ptr<IOutlinerDecorator>> m_Decorators;
    std::vector<std::unique_ptr<IOutlinerContextMenuProvider>> m_ContextMenus;
    std::unordered_map<std::string, OutlinerFilterPreset> m_Presets;

    std::weak_ptr<::we::editor::contentbrowser::TreeView> m_Tree;
    bool m_ApplyingExternalSelection = false;
};

class WorldOutlinerRuntimeImpl final : public IWorldOutlinerRuntime {
public:
    explicit WorldOutlinerRuntimeImpl(WorldOutlinerDependencies deps)
        : m_Outliner(std::make_unique<WorldOutlinerImpl>(std::move(deps)))
    {}

    [[nodiscard]] IWorldOutliner& Outliner() noexcept override { return *m_Outliner; }
    [[nodiscard]] const IWorldOutliner& Outliner() const noexcept override { return *m_Outliner; }

    void Shutdown() override { m_Outliner.reset(); }

private:
    std::unique_ptr<WorldOutlinerImpl> m_Outliner;
};

} // namespace detail

std::unique_ptr<IWorldOutlinerRuntime> CreateWorldOutlinerRuntime(const WorldOutlinerDependencies& deps) {
    return std::make_unique<detail::WorldOutlinerRuntimeImpl>(deps);
}

} // namespace we::editor::outliner
