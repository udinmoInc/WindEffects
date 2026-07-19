#include "OutlinerInternal.h"

#include <algorithm>
#include <utility>

namespace we::editor::outliner {
namespace detail {
namespace {

class TreeModelImpl final : public IOutlinerTreeModel {
public:
    void SetProviders(std::vector<IOutlinerDataProvider*> providers) { m_Providers = std::move(providers); }
    void SetFilters(std::vector<IOutlinerFilter*> filters) { m_Filters = std::move(filters); }
    void SetSearch(IOutlinerSearch* search) { m_Search = search; }

    void Rebuild() override {
        const auto t0 = NowMicros();
        m_Visible.clear();
        m_Depth.clear();
        std::uint64_t nodes = 0;

        for (auto* provider : m_Providers) {
            if (!provider) {
                continue;
            }
            provider->Rebuild();
            for (const auto rootId : provider->GetRootIds()) {
                CollectVisible(provider, rootId, 0, nodes);
            }
        }

        m_Dirty = false;
        WorldOutlinerDiagnostics::Get().OnRebuild(NowMicros() - t0, nodes, m_Visible.size());
    }

    void MarkDirty() override { m_Dirty = true; }
    [[nodiscard]] bool IsDirty() const noexcept override { return m_Dirty; }

    [[nodiscard]] std::span<const OutlinerNodeId> GetVisibleRows() const noexcept override { return m_Visible; }

    [[nodiscard]] const IOutlinerNode* GetNode(OutlinerNodeId id) const override {
        for (auto* provider : m_Providers) {
            if (auto* node = provider->FindNode(id)) {
                return node;
            }
        }
        return nullptr;
    }

    [[nodiscard]] std::uint16_t GetDepth(OutlinerNodeId id) const noexcept override {
        auto it = m_Depth.find(id.value);
        return it != m_Depth.end() ? it->second : 0;
    }

    void SetExpanded(OutlinerNodeId id, bool expanded) override {
        m_Expanded[id.value] = expanded;
        MarkDirty();
    }

    [[nodiscard]] bool IsExpanded(OutlinerNodeId id) const noexcept override {
        auto it = m_Expanded.find(id.value);
        if (it != m_Expanded.end()) {
            return it->second;
        }
        if (const auto* node = GetNode(id)) {
            return node->GetFlags().expanded;
        }
        return false;
    }

    void ExpandTo(OutlinerNodeId id) override {
        const auto* node = GetNode(id);
        while (node && node->GetParentId().IsValid()) {
            SetExpanded(node->GetParentId(), true);
            node = GetNode(node->GetParentId());
        }
        MarkDirty();
    }

    void SetFilterState(const OutlinerFilterState& state) override {
        m_FilterState = state;
        if (m_Search) {
            m_Search->SetQuery(state.searchQuery);
        }
        MarkDirty();
    }

    [[nodiscard]] const OutlinerFilterState& GetFilterState() const noexcept override { return m_FilterState; }

private:
    void CollectVisible(
        IOutlinerDataProvider* provider,
        OutlinerNodeId id,
        std::uint16_t depth,
        std::uint64_t& nodeCount)
    {
        const auto* node = provider->FindNode(id);
        if (!node) {
            return;
        }
        ++nodeCount;

        bool passes = true;
        for (auto* filter : m_Filters) {
            if (filter && !filter->Passes(*node, m_FilterState)) {
                passes = false;
                break;
            }
        }
        if (passes && m_Search && !m_Search->Matches(*node)) {
            passes = false;
        }

        if (passes) {
            m_Visible.push_back(id);
            m_Depth[id.value] = depth;
        }

        const bool expand = IsExpanded(id) || !m_FilterState.searchQuery.empty();
        if (!expand) {
            return;
        }
        if (node->GetFlags().lazyChildren) {
            provider->EnsureChildrenLoaded(id);
        }
        for (const auto child : node->GetChildren()) {
            CollectVisible(provider, child, static_cast<std::uint16_t>(depth + 1), nodeCount);
        }
    }

    std::vector<IOutlinerDataProvider*> m_Providers;
    std::vector<IOutlinerFilter*> m_Filters;
    IOutlinerSearch* m_Search = nullptr;
    OutlinerFilterState m_FilterState{};
    std::vector<OutlinerNodeId> m_Visible;
    std::unordered_map<std::uint64_t, std::uint16_t> m_Depth;
    std::unordered_map<std::uint64_t, bool> m_Expanded;
    bool m_Dirty = true;
};

class FolderProviderImpl final : public IOutlinerFolderProvider {
public:
    FolderProviderImpl(scene::Scene* scene, IOutlinerEventRouter& events, undo::ITransactionManager* tx)
        : m_Scene(scene)
        , m_Events(events)
        , m_Tx(tx)
    {}

    [[nodiscard]] OutlinerNodeId CreateFolder(std::string_view name, OutlinerNodeId parent) override {
        if (!m_Scene) {
            return {};
        }
        m_Scene->CreateEntity(std::string(name), scene::EntityType::EmptyActor);
        auto& entities = m_Scene->GetEntities();
        if (entities.empty()) {
            return {};
        }
        auto& folder = entities.back();
        folder.ParentId = parent.value;
        const OutlinerNodeId id{folder.Id};

        if (m_Tx) {
            const auto entityId = folder.Id;
            const auto parentId = folder.ParentId;
            const std::string folderName = folder.Name;
            (void)m_Tx->RecordCustom(
                "Create Folder",
                undo::TransactionKind::ObjectCreate,
                "outliner.create_folder",
                [scene = m_Scene, entityId]() {
                    const int idx = scene->FindEntityIndexById(entityId);
                    if (idx >= 0) {
                        scene->DestroyEntity(static_cast<size_t>(idx));
                    }
                    return true;
                },
                [scene = m_Scene, folderName, parentId]() {
                    scene->CreateEntity(folderName, scene::EntityType::EmptyActor);
                    auto& ents = scene->GetEntities();
                    if (!ents.empty()) {
                        ents.back().ParentId = parentId;
                    }
                    return true;
                });
            WorldOutlinerDiagnostics::Get().OnTransaction();
        }

        OutlinerEvent ev;
        ev.kind = OutlinerEventKind::HierarchyChanged;
        ev.primary = id;
        m_Events.Publish(ev);
        return id;
    }

    [[nodiscard]] bool RenameFolder(OutlinerNodeId id, std::string_view name) override {
        if (!m_Scene) {
            return false;
        }
        auto* entity = m_Scene->FindEntityById(id.value);
        if (!entity) {
            return false;
        }
        const std::string before = entity->Name;
        const std::string after(name);
        entity->Name = after;
        if (m_Tx) {
            (void)m_Tx->RecordCustom(
                "Rename Folder",
                undo::TransactionKind::Rename,
                "outliner.rename",
                [scene = m_Scene, id, before]() {
                    if (auto* e = scene->FindEntityById(id.value)) {
                        e->Name = before;
                    }
                    return true;
                },
                [scene = m_Scene, id, after]() {
                    if (auto* e = scene->FindEntityById(id.value)) {
                        e->Name = after;
                    }
                    return true;
                });
            WorldOutlinerDiagnostics::Get().OnTransaction();
        }
        OutlinerEvent ev;
        ev.kind = OutlinerEventKind::NodeRenamed;
        ev.primary = id;
        m_Events.Publish(ev);
        return true;
    }

    [[nodiscard]] bool MoveFolder(OutlinerNodeId id, OutlinerNodeId newParent) override {
        if (!m_Scene) {
            return false;
        }
        auto* entity = m_Scene->FindEntityById(id.value);
        if (!entity) {
            return false;
        }
        const auto before = entity->ParentId;
        entity->ParentId = newParent.value;
        if (m_Tx) {
            (void)m_Tx->RecordCustom(
                "Move Folder",
                undo::TransactionKind::Reparent,
                "outliner.reparent",
                [scene = m_Scene, id, before]() {
                    if (auto* e = scene->FindEntityById(id.value)) {
                        e->ParentId = before;
                    }
                    return true;
                },
                [scene = m_Scene, id, parent = newParent.value]() {
                    if (auto* e = scene->FindEntityById(id.value)) {
                        e->ParentId = parent;
                    }
                    return true;
                });
            WorldOutlinerDiagnostics::Get().OnTransaction();
        }
        OutlinerEvent ev;
        ev.kind = OutlinerEventKind::NodeReparented;
        ev.primary = id;
        m_Events.Publish(ev);
        return true;
    }

    [[nodiscard]] bool DeleteFolder(OutlinerNodeId id) override {
        if (!m_Scene) {
            return false;
        }
        const int idx = m_Scene->FindEntityIndexById(id.value);
        if (idx < 0) {
            return false;
        }
        // Snapshot minimal undo: recreate empty folder (children stay orphaned → root via Scene).
        const auto* entity = m_Scene->FindEntityById(id.value);
        const std::string name = entity ? entity->Name : "Folder";
        const auto parentId = entity ? entity->ParentId : 0;
        m_Scene->DestroyEntity(static_cast<size_t>(idx));
        if (m_Tx) {
            (void)m_Tx->RecordCustom(
                "Delete Folder",
                undo::TransactionKind::ObjectDelete,
                "outliner.delete_folder",
                [scene = m_Scene, name, parentId]() {
                    scene->CreateEntity(name, scene::EntityType::EmptyActor);
                    auto& ents = scene->GetEntities();
                    if (!ents.empty()) {
                        ents.back().ParentId = parentId;
                    }
                    return true;
                },
                [scene = m_Scene, name]() {
                    const int i = scene->FindEntityIndexByName(name);
                    if (i >= 0) {
                        scene->DestroyEntity(static_cast<size_t>(i));
                    }
                    return true;
                });
            WorldOutlinerDiagnostics::Get().OnTransaction();
        }
        OutlinerEvent ev;
        ev.kind = OutlinerEventKind::NodeDeleted;
        ev.primary = id;
        m_Events.Publish(ev);
        return true;
    }

private:
    scene::Scene* m_Scene = nullptr;
    IOutlinerEventRouter& m_Events;
    undo::ITransactionManager* m_Tx = nullptr;
};

class DragDropHandlerImpl final : public IOutlinerDragDropHandler {
public:
    explicit DragDropHandlerImpl(IOutlinerCommandRouter& commands)
        : m_Commands(commands)
    {}

    [[nodiscard]] bool CanDrop(OutlinerNodeId dragged, OutlinerNodeId target) const override {
        return dragged.IsValid() && target.IsValid() && dragged != target;
    }

    [[nodiscard]] bool Drop(OutlinerNodeId dragged, OutlinerNodeId target) override {
        if (!CanDrop(dragged, target)) {
            return false;
        }
        return m_Commands.Reparent(dragged, target);
    }

private:
    IOutlinerCommandRouter& m_Commands;
};

class CommandRouterImpl final : public IOutlinerCommandRouter {
public:
    CommandRouterImpl(
        WorldOutlinerDependencies deps,
        IOutlinerSelection& selection,
        IOutlinerFolderProvider& folders,
        IOutlinerEventRouter& events,
        IOutlinerTreeModel& model)
        : m_Deps(std::move(deps))
        , m_Selection(selection)
        , m_Folders(folders)
        , m_Events(events)
        , m_Model(model)
    {}

    [[nodiscard]] undo::ITransactionManager* Tx() const {
        return m_Deps.undo ? &m_Deps.undo->Manager() : nullptr;
    }

    [[nodiscard]] bool Rename(OutlinerNodeId id, std::string_view newName) override {
        WorldOutlinerDiagnostics::Get().OnCommand();
        if (!m_Deps.scene) {
            return false;
        }
        auto* entity = m_Deps.scene->FindEntityById(id.value);
        if (!entity) {
            return false;
        }
        if (entity->Type == scene::EntityType::EmptyActor) {
            return m_Folders.RenameFolder(id, newName);
        }
        const std::string before = entity->Name;
        const std::string after(newName);
        entity->Name = after;
        if (auto* tx = Tx()) {
            (void)tx->RecordCustom(
                "Rename Actor",
                undo::TransactionKind::Rename,
                "outliner.rename",
                [scene = m_Deps.scene, id, before]() {
                    if (auto* e = scene->FindEntityById(id.value)) {
                        e->Name = before;
                    }
                    return true;
                },
                [scene = m_Deps.scene, id, after]() {
                    if (auto* e = scene->FindEntityById(id.value)) {
                        e->Name = after;
                    }
                    return true;
                });
            WorldOutlinerDiagnostics::Get().OnTransaction();
        }
        OutlinerEvent ev;
        ev.kind = OutlinerEventKind::NodeRenamed;
        ev.primary = id;
        m_Events.Publish(ev);
        return true;
    }

    [[nodiscard]] bool DeleteSelected() override {
        WorldOutlinerDiagnostics::Get().OnCommand();
        if (!m_Deps.scene || m_Selection.IsEmpty()) {
            return false;
        }
        auto selected = std::vector<OutlinerNodeId>(
            m_Selection.GetSelected().begin(), m_Selection.GetSelected().end());
        for (const auto id : selected) {
            const int idx = m_Deps.scene->FindEntityIndexById(id.value);
            if (idx < 0) {
                continue;
            }
            const auto* entity = m_Deps.scene->FindEntityById(id.value);
            const std::string name = entity ? entity->Name : "Actor";
            const auto type = entity ? entity->Type : scene::EntityType::EmptyActor;
            const auto parentId = entity ? entity->ParentId : 0;
            const auto pos = entity ? entity->Position : we::math::Vec3{};
            m_Deps.scene->DestroyEntity(static_cast<size_t>(idx));
            if (auto* tx = Tx()) {
                (void)tx->RecordCustom(
                    "Delete Actor",
                    undo::TransactionKind::ActorDestroy,
                    "outliner.delete",
                    [scene = m_Deps.scene, name, type, parentId, pos]() {
                        scene->CreateEntity(name, type);
                        auto& ents = scene->GetEntities();
                        if (!ents.empty()) {
                            ents.back().ParentId = parentId;
                            ents.back().Position = pos;
                        }
                        return true;
                    },
                    [scene = m_Deps.scene, name]() {
                        const int i = scene->FindEntityIndexByName(name);
                        if (i >= 0) {
                            scene->DestroyEntity(static_cast<size_t>(i));
                        }
                        return true;
                    });
                WorldOutlinerDiagnostics::Get().OnTransaction();
            }
        }
        m_Selection.Clear();
        OutlinerEvent ev;
        ev.kind = OutlinerEventKind::NodeDeleted;
        m_Events.Publish(ev);
        return true;
    }

    [[nodiscard]] bool DuplicateSelected() override {
        WorldOutlinerDiagnostics::Get().OnCommand();
        if (!m_Deps.scene || m_Selection.IsEmpty()) {
            return false;
        }
        const auto primary = m_Selection.GetPrimary();
        const auto* src = m_Deps.scene->FindEntityById(primary.value);
        if (!src) {
            return false;
        }
        const auto copy = *src;
        m_Deps.scene->CreateEntity(copy.Name + "_Copy", copy.Type);
        auto& ents = m_Deps.scene->GetEntities();
        if (ents.empty()) {
            return false;
        }
        auto& dst = ents.back();
        dst.ParentId = copy.ParentId;
        dst.Position = copy.Position;
        dst.Rotation = copy.Rotation;
        dst.Scale = copy.Scale;
        dst.Color = copy.Color;
        if (auto* tx = Tx()) {
            const auto newId = dst.Id;
            (void)tx->RecordCustom(
                "Duplicate Actor",
                undo::TransactionKind::ActorSpawn,
                "outliner.duplicate",
                [scene = m_Deps.scene, newId]() {
                    const int i = scene->FindEntityIndexById(newId);
                    if (i >= 0) {
                        scene->DestroyEntity(static_cast<size_t>(i));
                    }
                    return true;
                },
                [scene = m_Deps.scene, copy]() {
                    scene->CreateEntity(copy.Name + "_Copy", copy.Type);
                    auto& e = scene->GetEntities();
                    if (!e.empty()) {
                        e.back().ParentId = copy.ParentId;
                        e.back().Position = copy.Position;
                    }
                    return true;
                });
            WorldOutlinerDiagnostics::Get().OnTransaction();
        }
        OutlinerEvent ev;
        ev.kind = OutlinerEventKind::HierarchyChanged;
        m_Events.Publish(ev);
        return true;
    }

    [[nodiscard]] bool Reparent(OutlinerNodeId child, OutlinerNodeId newParent) override {
        WorldOutlinerDiagnostics::Get().OnCommand();
        if (!m_Deps.scene) {
            return false;
        }
        auto* entity = m_Deps.scene->FindEntityById(child.value);
        if (!entity) {
            return false;
        }
        const auto before = entity->ParentId;
        entity->ParentId = newParent.value;
        if (auto* tx = Tx()) {
            (void)tx->RecordCustom(
                "Reparent Actor",
                undo::TransactionKind::Reparent,
                "outliner.reparent",
                [scene = m_Deps.scene, child, before]() {
                    if (auto* e = scene->FindEntityById(child.value)) {
                        e->ParentId = before;
                    }
                    return true;
                },
                [scene = m_Deps.scene, child, parent = newParent.value]() {
                    if (auto* e = scene->FindEntityById(child.value)) {
                        e->ParentId = parent;
                    }
                    return true;
                });
            WorldOutlinerDiagnostics::Get().OnTransaction();
        }
        OutlinerEvent ev;
        ev.kind = OutlinerEventKind::NodeReparented;
        ev.primary = child;
        m_Events.Publish(ev);
        return true;
    }

    [[nodiscard]] bool CopySelected() override {
        WorldOutlinerDiagnostics::Get().OnCommand();
        m_Clipboard.assign(m_Selection.GetSelected().begin(), m_Selection.GetSelected().end());
        return !m_Clipboard.empty();
    }

    [[nodiscard]] bool Paste(OutlinerNodeId parent) override {
        WorldOutlinerDiagnostics::Get().OnCommand();
        if (m_Clipboard.empty()) {
            return false;
        }
        m_Selection.Set(m_Clipboard.front());
        const bool ok = DuplicateSelected();
        if (ok && parent.IsValid() && m_Deps.scene) {
            auto& ents = m_Deps.scene->GetEntities();
            if (!ents.empty()) {
                ents.back().ParentId = parent.value;
            }
        }
        return ok;
    }

    [[nodiscard]] bool FocusSelected() override {
        WorldOutlinerDiagnostics::Get().OnCommand();
        if (!m_Deps.viewportEdit || m_Selection.IsEmpty()) {
            return false;
        }
        std::vector<viewportedit::ViewportObjectId> ids;
        for (const auto id : m_Selection.GetSelected()) {
            ids.push_back(viewportedit::ViewportObjectId{id.value});
        }
        m_Deps.viewportEdit->Camera().FrameSelection(ids);
        return true;
    }

    [[nodiscard]] bool RevealInViewport() override { return FocusSelected(); }

    [[nodiscard]] bool RevealInContentBrowser() override {
        WorldOutlinerDiagnostics::Get().OnCommand();
        // AssetRuntime / ContentBrowser reveal hook reserved.
        return false;
    }

    [[nodiscard]] bool CreateFolder(std::string_view name) override {
        WorldOutlinerDiagnostics::Get().OnCommand();
        const auto parent = m_Selection.GetPrimary();
        const auto id = m_Folders.CreateFolder(name, parent);
        return id.IsValid();
    }

    [[nodiscard]] bool SetVisible(OutlinerNodeId id, bool visible) override {
        WorldOutlinerDiagnostics::Get().OnCommand();
        m_Visibility[id.value] = visible;
        (void)m_Model;
        return true;
    }

    [[nodiscard]] bool SetLocked(OutlinerNodeId id, bool locked) override {
        WorldOutlinerDiagnostics::Get().OnCommand();
        m_Locked[id.value] = locked;
        return true;
    }

    [[nodiscard]] bool ToggleFavorite(OutlinerNodeId id) override {
        WorldOutlinerDiagnostics::Get().OnCommand();
        m_Favorites[id.value] = !m_Favorites[id.value];
        return true;
    }

    [[nodiscard]] bool TogglePinned(OutlinerNodeId id) override {
        WorldOutlinerDiagnostics::Get().OnCommand();
        m_Pinned[id.value] = !m_Pinned[id.value];
        return true;
    }

private:
    WorldOutlinerDependencies m_Deps;
    IOutlinerSelection& m_Selection;
    IOutlinerFolderProvider& m_Folders;
    IOutlinerEventRouter& m_Events;
    IOutlinerTreeModel& m_Model;
    std::vector<OutlinerNodeId> m_Clipboard;
    std::unordered_map<std::uint64_t, bool> m_Visibility;
    std::unordered_map<std::uint64_t, bool> m_Locked;
    std::unordered_map<std::uint64_t, bool> m_Favorites;
    std::unordered_map<std::uint64_t, bool> m_Pinned;
};

} // namespace

// Factories used by Runtime
std::unique_ptr<IOutlinerTreeModel> CreateTreeModel() {
    return std::make_unique<TreeModelImpl>();
}
std::unique_ptr<IOutlinerFolderProvider> CreateFolderProvider(
    scene::Scene* scene, IOutlinerEventRouter& events, undo::ITransactionManager* tx)
{
    return std::make_unique<FolderProviderImpl>(scene, events, tx);
}
std::unique_ptr<IOutlinerCommandRouter> CreateCommandRouter(
    WorldOutlinerDependencies deps,
    IOutlinerSelection& selection,
    IOutlinerFolderProvider& folders,
    IOutlinerEventRouter& events,
    IOutlinerTreeModel& model)
{
    return std::make_unique<CommandRouterImpl>(std::move(deps), selection, folders, events, model);
}
std::unique_ptr<IOutlinerDragDropHandler> CreateDragDrop(IOutlinerCommandRouter& commands) {
    return std::make_unique<DragDropHandlerImpl>(commands);
}

void TreeModelSetProviders(IOutlinerTreeModel& model, std::vector<IOutlinerDataProvider*> providers) {
    if (auto* impl = dynamic_cast<TreeModelImpl*>(&model)) {
        impl->SetProviders(std::move(providers));
    }
}
void TreeModelSetFilters(IOutlinerTreeModel& model, std::vector<IOutlinerFilter*> filters) {
    if (auto* impl = dynamic_cast<TreeModelImpl*>(&model)) {
        impl->SetFilters(std::move(filters));
    }
}
void TreeModelSetSearch(IOutlinerTreeModel& model, IOutlinerSearch* search) {
    if (auto* impl = dynamic_cast<TreeModelImpl*>(&model)) {
        impl->SetSearch(search);
    }
}

} // namespace detail
} // namespace we::editor::outliner
