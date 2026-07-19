#include "ContentBrowser/CreateContentBrowserRuntime.h"
#include "ContentBrowser/ContentBrowserDiagnostics.h"
#include "ContentBrowser/IContentBrowser.h"
#include "ContentBrowser/IAssetCommands.h"
#include "ContentBrowser/IAssetServices.h"

#include "Services/ContentBrowserService.h"
#include "Registry/ContentAssetRegistry.h"
#include "Registry/AssetTypes.h"
#include "ContentBrowser/AssetImportBridge.h"
#include "Core/Logger.h"
#include "Platform/PlatformSDK.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace we::editor::contentbrowser {
namespace detail {

using clock = std::chrono::steady_clock;

std::uint64_t NowMicros() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(clock::now().time_since_epoch()).count());
}

ContentAssetKind KindFromAssetType(AssetType type) {
    switch (type) {
    case AssetType::Folder: return ContentAssetKind::Folder;
    case AssetType::Texture: return ContentAssetKind::Texture;
    case AssetType::Material: return ContentAssetKind::Material;
    case AssetType::MaterialInstance: return ContentAssetKind::MaterialInstance;
    case AssetType::StaticMesh: return ContentAssetKind::StaticMesh;
    case AssetType::SkeletalMesh: return ContentAssetKind::SkeletalMesh;
    case AssetType::Animation: return ContentAssetKind::Animation;
    case AssetType::Blueprint: return ContentAssetKind::Blueprint;
    case AssetType::Scene: return ContentAssetKind::Scene;
    case AssetType::Prefab: return ContentAssetKind::Prefab;
    case AssetType::Audio: return ContentAssetKind::Audio;
    case AssetType::Font: return ContentAssetKind::Font;
    case AssetType::Script: return ContentAssetKind::Script;
    case AssetType::Video: return ContentAssetKind::Video;
    default: return ContentAssetKind::Unknown;
    }
}

std::string ToLower(std::string_view in) {
    std::string out(in);
    for (char& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

class AssetNodeAdapter final : public IContentAssetNode {
public:
    explicit AssetNodeAdapter(const AssetRecord* record)
        : m_Record(record)
    {}

    [[nodiscard]] ContentAssetId GetId() const override {
        return ContentAssetId{m_Record ? m_Record->id : std::string{}};
    }
    [[nodiscard]] std::string_view GetName() const noexcept override {
        return m_Record ? std::string_view{m_Record->name} : std::string_view{};
    }
    [[nodiscard]] std::string_view GetVirtualPath() const noexcept override {
        return m_Record ? std::string_view{m_Record->virtualPath} : std::string_view{};
    }
    [[nodiscard]] std::string_view GetDiskPath() const noexcept override {
        return m_Record ? std::string_view{m_Record->diskPath} : std::string_view{};
    }
    [[nodiscard]] ContentAssetKind GetKind() const noexcept override {
        return m_Record ? KindFromAssetType(m_Record->type) : ContentAssetKind::Unknown;
    }
    [[nodiscard]] bool IsFolder() const noexcept override { return m_Record && m_Record->isFolder; }
    [[nodiscard]] bool IsFavorite() const noexcept override { return m_Record && m_Record->isFavorite; }
    [[nodiscard]] std::span<const std::string> GetTags() const noexcept override { return m_Tags; }
    [[nodiscard]] std::uint64_t GetModifiedTime() const noexcept override {
        return m_Record ? m_Record->modifiedTime : 0;
    }
    [[nodiscard]] std::uint64_t GetFileSize() const noexcept override {
        return m_Record ? m_Record->fileSize : 0;
    }

private:
    const AssetRecord* m_Record = nullptr;
    std::vector<std::string> m_Tags;
};

class FilesystemDataProvider final : public IAssetDataProvider {
public:
    [[nodiscard]] std::string_view GetName() const noexcept override { return "FilesystemCatalog"; }

    [[nodiscard]] std::vector<ContentAssetId> GetRootFolders() const override {
        return {ContentAssetId{"/Game"}};
    }

    [[nodiscard]] std::vector<ContentAssetId> GetChildren(const ContentAssetId& folder) const override {
        std::vector<ContentAssetId> ids;
        const auto path = folder.value.empty() ? "/Game" : folder.value;
        for (const auto* child : ContentAssetRegistry::Get().GetChildren(path)) {
            ids.push_back(ContentAssetId{child->id});
        }
        return ids;
    }

    [[nodiscard]] const IContentAssetNode* Find(const ContentAssetId& id) const override {
        const auto* record = ContentAssetRegistry::Get().FindById(id.value);
        if (!record) {
            return nullptr;
        }
        m_Cache[id.value] = std::make_unique<AssetNodeAdapter>(record);
        return m_Cache[id.value].get();
    }

    [[nodiscard]] const IContentAssetNode* FindByPath(std::string_view virtualPath) const override {
        const auto* record = ContentAssetRegistry::Get().FindByVirtualPath(std::string(virtualPath));
        if (!record) {
            return nullptr;
        }
        m_Cache[record->id] = std::make_unique<AssetNodeAdapter>(record);
        return m_Cache[record->id].get();
    }

    void Rebuild() override {
        m_Cache.clear();
        ContentAssetRegistry::Get().Refresh();
    }

private:
    mutable std::unordered_map<std::string, std::unique_ptr<AssetNodeAdapter>> m_Cache;
};

class RegistryProviderImpl final : public IAssetRegistryProvider {
public:
    explicit RegistryProviderImpl(std::unique_ptr<IAssetDataProvider> data)
        : m_Data(std::move(data))
    {}

    void Refresh() override { m_Data->Rebuild(); }
    void Tick(float dt) override { ContentAssetRegistry::Get().Tick(dt); }
    [[nodiscard]] std::size_t GetAssetCount() const noexcept override {
        return ContentAssetRegistry::Get().GetAllAssets().size();
    }
    [[nodiscard]] IAssetDataProvider& Data() noexcept override { return *m_Data; }

private:
    std::unique_ptr<IAssetDataProvider> m_Data;
};

class EventRouterImpl final : public IContentBrowserEventRouter {
public:
    void Publish(const ContentBrowserEvent& event) override {
        if (m_Suspended) {
            return;
        }
        for (auto& listener : m_Listeners) {
            listener(event);
        }
    }
    void Subscribe(ContentBrowserEventListener listener) override {
        if (listener) {
            m_Listeners.push_back(std::move(listener));
        }
    }
    void Suspend(bool suspend) override { m_Suspended = suspend; }

private:
    std::vector<ContentBrowserEventListener> m_Listeners;
    bool m_Suspended = false;
};

class SelectionImpl final : public IAssetSelection {
public:
    explicit SelectionImpl(IContentBrowserEventRouter& events)
        : m_Events(events)
    {}

    [[nodiscard]] std::span<const ContentAssetId> GetSelected() const noexcept override { return m_Selected; }
    [[nodiscard]] ContentAssetId GetPrimary() const override {
        return m_Selected.empty() ? ContentAssetId{} : m_Selected.front();
    }
    [[nodiscard]] bool IsSelected(const ContentAssetId& id) const noexcept override {
        return std::find(m_Selected.begin(), m_Selected.end(), id) != m_Selected.end();
    }
    [[nodiscard]] bool IsEmpty() const noexcept override { return m_Selected.empty(); }

    void Clear() override {
        if (m_Selected.empty()) {
            return;
        }
        m_Selected.clear();
        Notify();
    }

    void Set(const ContentAssetId& id) override {
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

    void SetMany(std::span<const ContentAssetId> ids) override {
        m_Selected.assign(ids.begin(), ids.end());
        Notify();
    }

    void Apply(ContentSelectionOp op, const ContentAssetId& id) override {
        if (!id.IsValid()) {
            return;
        }
        switch (op) {
        case ContentSelectionOp::Replace:
            Set(id);
            return;
        case ContentSelectionOp::Add:
            if (!IsSelected(id)) {
                m_Selected.push_back(id);
                Notify();
            }
            return;
        case ContentSelectionOp::Toggle:
            if (IsSelected(id)) {
                m_Selected.erase(std::remove(m_Selected.begin(), m_Selected.end(), id), m_Selected.end());
            } else {
                m_Selected.push_back(id);
            }
            Notify();
            return;
        case ContentSelectionOp::Remove:
            if (IsSelected(id)) {
                m_Selected.erase(std::remove(m_Selected.begin(), m_Selected.end(), id), m_Selected.end());
                Notify();
            }
            return;
        }
    }

private:
    void Notify() {
        ContentBrowserDiagnostics::Get().OnSelectionChanged(m_Selected.size());
        ContentBrowserEvent ev;
        ev.kind = ContentEventKind::SelectionChanged;
        ev.assets = m_Selected;
        ev.primary = GetPrimary();
        m_Events.Publish(ev);
    }

    IContentBrowserEventRouter& m_Events;
    std::vector<ContentAssetId> m_Selected;
};

class SearchImpl final : public IAssetSearchProvider {
public:
    void SetQuery(std::string_view query) override {
        m_Query = std::string(query);
        m_Lower = ToLower(m_Query);
    }
    [[nodiscard]] std::string_view GetQuery() const noexcept override { return m_Query; }
    [[nodiscard]] bool Matches(const IContentAssetNode& node) const override {
        ContentBrowserDiagnostics::Get().OnSearchPass();
        if (m_Lower.empty()) {
            return true;
        }
        return ToLower(node.GetName()).find(m_Lower) != std::string::npos
            || ToLower(node.GetVirtualPath()).find(m_Lower) != std::string::npos;
    }

private:
    std::string m_Query;
    std::string m_Lower;
};

class DefaultFilter final : public IAssetFilterProvider {
public:
    [[nodiscard]] bool Passes(const IContentAssetNode& node, const ContentFilterState& state) const override {
        if (state.favoritesOnly && !node.IsFavorite()) {
            return false;
        }
        if (!state.showFolders && node.IsFolder()) {
            return false;
        }
        switch (node.GetKind()) {
        case ContentAssetKind::Texture:
            return state.showTextures;
        case ContentAssetKind::StaticMesh:
        case ContentAssetKind::SkeletalMesh:
            return state.showMeshes;
        case ContentAssetKind::Material:
        case ContentAssetKind::MaterialInstance:
            return state.showMaterials;
        case ContentAssetKind::Blueprint:
            return state.showBlueprints;
        case ContentAssetKind::Audio:
            return state.showAudio;
        case ContentAssetKind::Folder:
            return state.showFolders;
        default:
            return state.showOther;
        }
    }
};

class SorterImpl final : public IAssetSorter {
public:
    explicit SorterImpl(IAssetTreeModel& model)
        : m_Model(model)
    {}

    void Sort(std::vector<ContentAssetId>& ids, ContentSortMode mode) const override {
        std::sort(ids.begin(), ids.end(), [&](const ContentAssetId& a, const ContentAssetId& b) {
            const auto* na = m_Model.GetNode(a);
            const auto* nb = m_Model.GetNode(b);
            if (!na || !nb) {
                return a.value < b.value;
            }
            if (na->IsFolder() != nb->IsFolder()) {
                return na->IsFolder() > nb->IsFolder();
            }
            switch (mode) {
            case ContentSortMode::NameDesc:
                return na->GetName() > nb->GetName();
            case ContentSortMode::TypeAsc:
                return static_cast<int>(na->GetKind()) < static_cast<int>(nb->GetKind());
            case ContentSortMode::ModifiedDesc:
                return na->GetModifiedTime() > nb->GetModifiedTime();
            case ContentSortMode::SizeDesc:
                return na->GetFileSize() > nb->GetFileSize();
            case ContentSortMode::NameAsc:
            default:
                return na->GetName() < nb->GetName();
            }
        });
    }

private:
    IAssetTreeModel& m_Model;
};

class TreeModelImpl final : public IAssetTreeModel {
public:
    void SetRegistry(IAssetRegistryProvider* registry) { m_Registry = registry; }
    void SetSearch(IAssetSearchProvider* search) { m_Search = search; }
    void SetFilters(std::vector<IAssetFilterProvider*> filters) { m_Filters = std::move(filters); }
    void SetSorter(IAssetSorter* sorter) { m_Sorter = sorter; }

    void Rebuild() override {
        const auto t0 = NowMicros();
        m_Visible.clear();
        if (!m_Registry) {
            m_Dirty = false;
            return;
        }
        auto& data = m_Registry->Data();
        for (const auto& id : data.GetChildren(ContentAssetId{m_CurrentFolder})) {
            const auto* node = data.Find(id);
            if (!node) {
                continue;
            }
            bool passes = true;
            for (auto* filter : m_Filters) {
                if (filter && !filter->Passes(*node, m_Filter)) {
                    passes = false;
                    break;
                }
            }
            if (passes && m_Search && !m_Search->Matches(*node)) {
                passes = false;
            }
            if (passes) {
                m_Visible.push_back(id);
            }
        }
        if (m_Sorter) {
            m_Sorter->Sort(m_Visible, m_Filter.sortMode);
        }
        m_Dirty = false;
        ContentBrowserDiagnostics::Get().OnRefresh(
            NowMicros() - t0,
            m_Registry->GetAssetCount(),
            m_Visible.size());
    }

    void MarkDirty() override { m_Dirty = true; }
    [[nodiscard]] bool IsDirty() const noexcept override { return m_Dirty; }
    [[nodiscard]] std::span<const ContentAssetId> GetVisibleAssets() const noexcept override { return m_Visible; }
    [[nodiscard]] const IContentAssetNode* GetNode(const ContentAssetId& id) const override {
        return m_Registry ? m_Registry->Data().Find(id) : nullptr;
    }
    void SetFilterState(const ContentFilterState& state) override {
        m_Filter = state;
        if (m_Search) {
            m_Search->SetQuery(state.searchQuery);
        }
        MarkDirty();
    }
    [[nodiscard]] const ContentFilterState& GetFilterState() const noexcept override { return m_Filter; }
    void SetCurrentFolder(std::string_view virtualPath) override {
        m_CurrentFolder = virtualPath.empty() ? "/Game" : std::string(virtualPath);
        MarkDirty();
    }
    [[nodiscard]] std::string_view GetCurrentFolder() const noexcept override { return m_CurrentFolder; }

private:
    IAssetRegistryProvider* m_Registry = nullptr;
    IAssetSearchProvider* m_Search = nullptr;
    IAssetSorter* m_Sorter = nullptr;
    std::vector<IAssetFilterProvider*> m_Filters;
    ContentFilterState m_Filter{};
    std::vector<ContentAssetId> m_Visible;
    std::string m_CurrentFolder = "/Game";
    bool m_Dirty = true;
};

class ThumbnailProviderImpl final : public IAssetThumbnailProvider {
public:
    void Request(const ContentAssetId& id) override {
        ContentBrowserDiagnostics::Get().OnThumbnailRequest();
        ContentBrowserService::Get().RequestThumbnailForItem(id.value);
    }
    [[nodiscard]] we::rhi::RHIDescriptorSetHandle GetCached(const ContentAssetId& id) const override {
        return ContentBrowserService::Get().GetThumbnailManager().GetCachedTexture(id.value);
    }
    void SetVisible(std::span<const ContentAssetId> ids) override {
        std::unordered_set<std::string> visible;
        for (const auto& id : ids) {
            visible.insert(id.value);
        }
        ContentBrowserService::Get().SetVisibleItemIds(visible);
    }
    void Invalidate(const ContentAssetId& id) override {
        ContentBrowserService::Get().GetThumbnailManager().Invalidate(id.value);
    }
    void Tick(float) override {}
};

class FolderProviderImpl final : public IAssetFolderProvider {
public:
    explicit FolderProviderImpl(ContentBrowserTransactionFn recordTx)
        : m_RecordTx(std::move(recordTx))
    {}

    [[nodiscard]] bool CreateFolder(std::string_view parentVirtualPath, std::string_view name) override {
        const auto* parent = ContentAssetRegistry::Get().FindByVirtualPath(std::string(parentVirtualPath));
        if (!parent || !parent->isFolder) {
            return false;
        }
        namespace fs = std::filesystem;
        const fs::path disk = fs::path(parent->diskPath) / std::string(name);
        std::error_code ec;
        if (!fs::create_directories(disk, ec)) {
            return false;
        }
        if (m_RecordTx) {
            const auto path = disk.string();
            (void)m_RecordTx(
                "Create Folder",
                [path]() {
                    std::error_code e;
                    std::filesystem::remove_all(path, e);
                    return true;
                },
                [path]() {
                    std::error_code e;
                    std::filesystem::create_directories(path, e);
                    return !e;
                });
            ContentBrowserDiagnostics::Get().OnTransaction();
        }
        ContentAssetRegistry::Get().Refresh();
        return true;
    }

    [[nodiscard]] bool RenameFolder(const ContentAssetId& id, std::string_view newName) override {
        return RenamePath(id, newName);
    }

    [[nodiscard]] bool MoveFolder(const ContentAssetId& id, std::string_view newParentVirtualPath) override {
        const auto* record = ContentAssetRegistry::Get().FindById(id.value);
        const auto* parent = ContentAssetRegistry::Get().FindByVirtualPath(std::string(newParentVirtualPath));
        if (!record || !parent) {
            return false;
        }
        namespace fs = std::filesystem;
        const fs::path from = record->diskPath;
        const fs::path to = fs::path(parent->diskPath) / from.filename();
        std::error_code ec;
        fs::rename(from, to, ec);
        if (ec) {
            return false;
        }
        if (m_RecordTx) {
            (void)m_RecordTx(
                "Move Folder",
                [from = to.string(), to = from.string()]() {
                    std::error_code e;
                    std::filesystem::rename(from, to, e);
                    return !e;
                },
                [from = from.string(), to = to.string()]() {
                    std::error_code e;
                    std::filesystem::rename(from, to, e);
                    return !e;
                });
            ContentBrowserDiagnostics::Get().OnTransaction();
        }
        ContentAssetRegistry::Get().Refresh();
        return true;
    }

    [[nodiscard]] bool DeleteFolder(const ContentAssetId& id) override {
        return DeletePath(id);
    }

    [[nodiscard]] bool RenamePath(const ContentAssetId& id, std::string_view newName) {
        const auto* record = ContentAssetRegistry::Get().FindById(id.value);
        if (!record) {
            return false;
        }
        namespace fs = std::filesystem;
        const fs::path from = record->diskPath;
        const fs::path to = from.parent_path() / std::string(newName);
        std::error_code ec;
        fs::rename(from, to, ec);
        if (ec) {
            return false;
        }
        if (m_RecordTx) {
            (void)m_RecordTx(
                "Rename Asset",
                [from = to.string(), to = from.string()]() {
                    std::error_code e;
                    std::filesystem::rename(from, to, e);
                    return !e;
                },
                [from = from.string(), to = to.string()]() {
                    std::error_code e;
                    std::filesystem::rename(from, to, e);
                    return !e;
                });
            ContentBrowserDiagnostics::Get().OnTransaction();
        }
        ContentAssetRegistry::Get().Refresh();
        return true;
    }

    [[nodiscard]] bool DeletePath(const ContentAssetId& id) {
        const auto* record = ContentAssetRegistry::Get().FindById(id.value);
        if (!record) {
            return false;
        }
        namespace fs = std::filesystem;
        const auto path = record->diskPath;
        const bool wasDir = record->isFolder;
        std::error_code ec;
        if (wasDir) {
            fs::remove_all(path, ec);
        } else {
            fs::remove(path, ec);
        }
        if (ec) {
            return false;
        }
        // Undo recreate is best-effort for empty files/folders.
        if (m_RecordTx) {
            (void)m_RecordTx(
                "Delete Asset",
                [path, wasDir]() {
                    std::error_code e;
                    if (wasDir) {
                        std::filesystem::create_directories(path, e);
                    } else {
                        std::ofstream(path).put('\n');
                    }
                    return true;
                },
                [path, wasDir]() {
                    std::error_code e;
                    if (wasDir) {
                        std::filesystem::remove_all(path, e);
                    } else {
                        std::filesystem::remove(path, e);
                    }
                    return true;
                });
            ContentBrowserDiagnostics::Get().OnTransaction();
        }
        ContentAssetRegistry::Get().Refresh();
        return true;
    }

private:
    ContentBrowserTransactionFn m_RecordTx;
};

class CollectionProviderImpl final : public IAssetCollectionProvider {
public:
    [[nodiscard]] std::vector<std::string> ListCollections() const override {
        std::vector<std::string> names;
        names.reserve(m_Collections.size());
        for (const auto& [name, _] : m_Collections) {
            names.push_back(name);
        }
        return names;
    }
    [[nodiscard]] bool CreateCollection(std::string_view name) override {
        m_Collections.emplace(std::string(name), std::vector<ContentAssetId>{});
        return true;
    }
    [[nodiscard]] bool AddToCollection(std::string_view name, const ContentAssetId& id) override {
        m_Collections[std::string(name)].push_back(id);
        return true;
    }
    [[nodiscard]] bool RemoveFromCollection(std::string_view name, const ContentAssetId& id) override {
        auto& ids = m_Collections[std::string(name)];
        ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
        return true;
    }
    [[nodiscard]] std::vector<ContentAssetId> GetCollectionAssets(std::string_view name) const override {
        auto it = m_Collections.find(std::string(name));
        return it != m_Collections.end() ? it->second : std::vector<ContentAssetId>{};
    }

private:
    std::unordered_map<std::string, std::vector<ContentAssetId>> m_Collections;
};

class HistoryImpl final : public IAssetHistory {
public:
    explicit HistoryImpl(std::uint32_t limit)
        : m_Limit(limit)
    {}

    void PushRecent(const ContentAssetId& id) override {
        if (!id.IsValid()) {
            return;
        }
        m_Recent.erase(std::remove(m_Recent.begin(), m_Recent.end(), id), m_Recent.end());
        m_Recent.insert(m_Recent.begin(), id);
        if (m_Recent.size() > m_Limit) {
            m_Recent.resize(m_Limit);
        }
    }
    [[nodiscard]] std::span<const ContentAssetId> GetRecent() const noexcept override { return m_Recent; }
    void AddBookmark(std::string_view name, std::string_view virtualPath) override {
        m_Bookmarks.emplace_back(std::string(name), std::string(virtualPath));
    }
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> GetBookmarks() const override {
        return m_Bookmarks;
    }

private:
    std::uint32_t m_Limit = 64;
    std::vector<ContentAssetId> m_Recent;
    std::vector<std::pair<std::string, std::string>> m_Bookmarks;
};

class ClipboardImpl final : public IAssetClipboard {
public:
    explicit ClipboardImpl(IAssetCommandRouter& commands)
        : m_Commands(commands)
    {}

    void Copy(std::span<const ContentAssetId> ids) override {
        m_Ids.assign(ids.begin(), ids.end());
        m_Cut = false;
    }
    void Cut(std::span<const ContentAssetId> ids) override {
        m_Ids.assign(ids.begin(), ids.end());
        m_Cut = true;
    }
    [[nodiscard]] bool CanPaste() const noexcept override { return !m_Ids.empty(); }
    [[nodiscard]] bool Paste(std::string_view targetFolder) override {
        if (m_Ids.empty()) {
            return false;
        }
        bool ok = false;
        for (const auto& id : m_Ids) {
            if (m_Cut) {
                ok = m_Commands.Move(id, targetFolder) || ok;
            } else {
                ok = m_Commands.Duplicate(id) || ok;
            }
        }
        if (m_Cut) {
            m_Ids.clear();
        }
        return ok;
    }

private:
    IAssetCommandRouter& m_Commands;
    std::vector<ContentAssetId> m_Ids;
    bool m_Cut = false;
};

class ValidatorImpl final : public IAssetValidator {
public:
    [[nodiscard]] bool Validate(const ContentAssetId& id, std::string& outMessage) const override {
        const auto* record = ContentAssetRegistry::Get().FindById(id.value);
        if (!record) {
            outMessage = "Asset not found";
            return false;
        }
        if (!record->isFolder && !std::filesystem::exists(record->diskPath)) {
            outMessage = "Missing on disk";
            return false;
        }
        outMessage = "OK";
        return true;
    }
};

class CommandRouterImpl final : public IAssetCommandRouter {
public:
    CommandRouterImpl(
        ContentBrowserDependencies deps,
        IAssetSelection& selection,
        IAssetFolderProvider& folders,
        IAssetTreeModel& model,
        IContentBrowserEventRouter& events,
        IAssetHistory& history,
        IAssetClipboard& clipboard)
        : m_Deps(std::move(deps))
        , m_Selection(selection)
        , m_Folders(folders)
        , m_Model(model)
        , m_Events(events)
        , m_History(history)
        , m_Clipboard(clipboard)
    {}

    [[nodiscard]] bool RecordTx(std::string_view label, std::function<bool()> undoFn, std::function<bool()> redoFn) {
        if (!m_Deps.recordTransaction) {
            return false;
        }
        const bool ok = m_Deps.recordTransaction(label, std::move(undoFn), std::move(redoFn));
        if (ok) {
            ContentBrowserDiagnostics::Get().OnTransaction();
        }
        return ok;
    }

    [[nodiscard]] bool Rename(const ContentAssetId& id, std::string_view newName) override {
        ContentBrowserDiagnostics::Get().OnCommand();
        auto* folders = dynamic_cast<FolderProviderImpl*>(&m_Folders);
        const bool ok = folders ? folders->RenamePath(id, newName) : false;
        if (ok) {
            ContentBrowserEvent ev;
            ev.kind = ContentEventKind::AssetRenamed;
            ev.primary = id;
            m_Events.Publish(ev);
            m_Model.MarkDirty();
        }
        return ok;
    }

    [[nodiscard]] bool Move(const ContentAssetId& id, std::string_view newParentVirtualPath) override {
        ContentBrowserDiagnostics::Get().OnCommand();
        const bool ok = m_Folders.MoveFolder(id, newParentVirtualPath);
        if (ok) {
            ContentBrowserEvent ev;
            ev.kind = ContentEventKind::AssetMoved;
            ev.primary = id;
            m_Events.Publish(ev);
            m_Model.MarkDirty();
        }
        return ok;
    }

    [[nodiscard]] bool Duplicate(const ContentAssetId& id) override {
        ContentBrowserDiagnostics::Get().OnCommand();
        const auto* record = ContentAssetRegistry::Get().FindById(id.value);
        if (!record || record->isFolder) {
            return false;
        }
        namespace fs = std::filesystem;
        const fs::path src = record->diskPath;
        const fs::path dst = src.parent_path() / (src.stem().string() + "_Copy" + src.extension().string());
        std::error_code ec;
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            return false;
        }
        if (m_Deps.recordTransaction) {
            (void)RecordTx(
                "Duplicate Asset",
                [path = dst.string()]() {
                    std::error_code e;
                    std::filesystem::remove(path, e);
                    return true;
                },
                [src = src.string(), dst = dst.string()]() {
                    std::error_code e;
                    std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, e);
                    return !e;
                });
        }
        ContentAssetRegistry::Get().Refresh();
        m_Model.MarkDirty();
        return true;
    }

    [[nodiscard]] bool DeleteSelected() override {
        ContentBrowserDiagnostics::Get().OnCommand();
        auto* folders = dynamic_cast<FolderProviderImpl*>(&m_Folders);
        if (!folders) {
            return false;
        }
        auto selected = std::vector<ContentAssetId>(m_Selection.GetSelected().begin(), m_Selection.GetSelected().end());
        bool ok = false;
        for (const auto& id : selected) {
            ok = folders->DeletePath(id) || ok;
        }
        m_Selection.Clear();
        if (ok) {
            ContentBrowserEvent ev;
            ev.kind = ContentEventKind::AssetDeleted;
            m_Events.Publish(ev);
            m_Model.MarkDirty();
        }
        return ok;
    }

    [[nodiscard]] bool CopySelected() override {
        ContentBrowserDiagnostics::Get().OnCommand();
        m_Clipboard.Copy(m_Selection.GetSelected());
        return !m_Selection.IsEmpty();
    }

    [[nodiscard]] bool Paste(std::string_view targetFolder) override {
        ContentBrowserDiagnostics::Get().OnCommand();
        return m_Clipboard.Paste(targetFolder);
    }

    [[nodiscard]] bool Import(std::string_view sourcePath, std::string_view targetFolder) override {
        ContentBrowserDiagnostics::Get().OnCommand();
        const auto* folder = ContentAssetRegistry::Get().FindByVirtualPath(std::string(targetFolder));
        const std::filesystem::path outDir = folder ? folder->diskPath : m_Deps.contentRoot;
        const auto result = we::programs::editor::ImportAssetToContent(sourcePath, outDir);
        if (!result.success) {
            return false;
        }
        if (m_Deps.recordTransaction) {
            (void)RecordTx("Import Asset", []() { return true; }, []() { return true; });
        }
        ContentAssetRegistry::Get().Refresh();
        ContentBrowserEvent ev;
        ev.kind = ContentEventKind::AssetImported;
        m_Events.Publish(ev);
        m_Model.MarkDirty();
        return true;
    }

    [[nodiscard]] bool Reimport(const ContentAssetId& id) override {
        ContentBrowserDiagnostics::Get().OnCommand();
        const auto* record = ContentAssetRegistry::Get().FindById(id.value);
        if (!record) {
            return false;
        }
        return Import(record->diskPath, record->parentPath);
    }

    [[nodiscard]] bool Export(const ContentAssetId& id, std::string_view destination) override {
        ContentBrowserDiagnostics::Get().OnCommand();
        const auto* record = ContentAssetRegistry::Get().FindById(id.value);
        if (!record || record->isFolder) {
            return false;
        }
        std::error_code ec;
        std::filesystem::copy_file(
            record->diskPath,
            destination,
            std::filesystem::copy_options::overwrite_existing,
            ec);
        return !ec;
    }

    [[nodiscard]] bool ValidateSelected() override {
        ContentBrowserDiagnostics::Get().OnCommand();
        ValidatorImpl validator;
        for (const auto& id : m_Selection.GetSelected()) {
            std::string msg;
            if (!validator.Validate(id, msg)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool OpenInEditor(const ContentAssetId& id) override {
        ContentBrowserDiagnostics::Get().OnCommand();
        m_History.PushRecent(id);
        // Future: route to Material/Animation/etc. editors via plugin registry.
        return Reveal(id);
    }

    [[nodiscard]] bool RevealInOS(const ContentAssetId& id) override {
        ContentBrowserDiagnostics::Get().OnCommand();
        const auto* record = ContentAssetRegistry::Get().FindById(id.value);
        if (!record) {
            return false;
        }
        we::platform::ProcessLaunchDesc desc{};
#if defined(_WIN32)
        desc.executable = "explorer.exe";
        desc.arguments = {"/select," + record->diskPath};
#else
        desc.executable = "xdg-open";
        desc.arguments = {record->diskPath};
#endif
        const auto result = we::platform::Platform::Get().LaunchProcess(desc);
        return result.Ok();
    }

    [[nodiscard]] bool Reveal(const ContentAssetId& id) override {
        ContentBrowserDiagnostics::Get().OnCommand();
        const auto* record = ContentAssetRegistry::Get().FindById(id.value);
        if (!record) {
            return false;
        }
        m_Model.SetCurrentFolder(record->isFolder ? record->virtualPath : record->parentPath);
        ContentBrowserService::Get().SetCurrentFolder(std::string(m_Model.GetCurrentFolder()));
        m_Selection.Set(id);
        m_History.PushRecent(id);
        m_Model.MarkDirty();
        return true;
    }

    [[nodiscard]] bool RevealPath(std::string_view virtualPath) override {
        ContentBrowserDiagnostics::Get().OnCommand();
        const auto* record = ContentAssetRegistry::Get().FindByVirtualPath(std::string(virtualPath));
        if (!record) {
            m_Model.SetCurrentFolder(virtualPath);
            ContentBrowserService::Get().SetCurrentFolder(std::string(virtualPath));
            m_Model.MarkDirty();
            return true;
        }
        return Reveal(ContentAssetId{record->id});
    }

    [[nodiscard]] bool ToggleFavorite(const ContentAssetId& id) override {
        ContentBrowserDiagnostics::Get().OnCommand();
        ContentAssetRegistry::Get().ToggleFavorite(id.value);
        m_Model.MarkDirty();
        return true;
    }

    [[nodiscard]] bool CreateFolder(std::string_view name) override {
        ContentBrowserDiagnostics::Get().OnCommand();
        return m_Folders.CreateFolder(m_Model.GetCurrentFolder(), name);
    }

    [[nodiscard]] bool Refresh() override {
        ContentBrowserDiagnostics::Get().OnCommand();
        ContentAssetRegistry::Get().Refresh();
        m_Model.MarkDirty();
        ContentBrowserEvent ev;
        ev.kind = ContentEventKind::CatalogRefreshed;
        m_Events.Publish(ev);
        return true;
    }

private:
    ContentBrowserDependencies m_Deps;
    IAssetSelection& m_Selection;
    IAssetFolderProvider& m_Folders;
    IAssetTreeModel& m_Model;
    IContentBrowserEventRouter& m_Events;
    IAssetHistory& m_History;
    IAssetClipboard& m_Clipboard;
};

class DragDropImpl final : public IAssetDragDropHandler {
public:
    explicit DragDropImpl(IAssetCommandRouter& commands)
        : m_Commands(commands)
    {}

    [[nodiscard]] bool CanDrop(const ContentAssetId& dragged, std::string_view targetFolder) const override {
        return dragged.IsValid() && !targetFolder.empty();
    }
    [[nodiscard]] bool Drop(const ContentAssetId& dragged, std::string_view targetFolder) override {
        return CanDrop(dragged, targetFolder) && m_Commands.Move(dragged, targetFolder);
    }
    [[nodiscard]] bool DropExternal(std::string_view sourcePath, std::string_view targetFolder) override {
        return m_Commands.Import(sourcePath, targetFolder);
    }

private:
    IAssetCommandRouter& m_Commands;
};

class NullClipboard final : public IAssetClipboard {
public:
    void Copy(std::span<const ContentAssetId>) override {}
    void Cut(std::span<const ContentAssetId>) override {}
    [[nodiscard]] bool CanPaste() const noexcept override { return false; }
    [[nodiscard]] bool Paste(std::string_view) override { return false; }
};

class ContentBrowserImpl final : public IContentBrowser {
public:
    explicit ContentBrowserImpl(ContentBrowserDependencies deps)
        : m_Deps(std::move(deps))
    {
        ContentBrowserService::Get().Initialize(
            m_Deps.iconRenderer,
            m_Deps.contentRoot.string());

        m_Events = std::make_unique<EventRouterImpl>();
        m_Selection = std::make_unique<SelectionImpl>(*m_Events);
        m_Search = std::make_unique<SearchImpl>();
        m_Model = std::make_unique<TreeModelImpl>();
        m_Sorter = std::make_unique<SorterImpl>(*m_Model);
        m_Registry = std::make_unique<RegistryProviderImpl>(std::make_unique<FilesystemDataProvider>());
        m_Folders = std::make_unique<FolderProviderImpl>(m_Deps.recordTransaction);
        m_Collections = std::make_unique<CollectionProviderImpl>();
        m_History = std::make_unique<HistoryImpl>(m_Deps.config.recentLimit);
        m_Thumbnails = std::make_unique<ThumbnailProviderImpl>();
        m_Clipboard = std::make_unique<NullClipboard>();

        RegisterFilter(std::make_unique<DefaultFilter>());

        m_Commands = std::make_unique<CommandRouterImpl>(
            m_Deps,
            *m_Selection,
            *m_Folders,
            *m_Model,
            *m_Events,
            *m_History,
            *m_Clipboard);
        m_Clipboard = std::make_unique<ClipboardImpl>(*m_Commands);
        m_Commands = std::make_unique<CommandRouterImpl>(
            m_Deps,
            *m_Selection,
            *m_Folders,
            *m_Model,
            *m_Events,
            *m_History,
            *m_Clipboard);
        m_DragDrop = std::make_unique<DragDropImpl>(*m_Commands);

        auto* model = static_cast<TreeModelImpl*>(m_Model.get());
        model->SetRegistry(m_Registry.get());
        model->SetSearch(m_Search.get());
        model->SetSorter(m_Sorter.get());
        std::vector<IAssetFilterProvider*> filters;
        for (auto& f : m_Filters) {
            filters.push_back(f.get());
        }
        model->SetFilters(std::move(filters));
        model->Rebuild();

        m_Events->Subscribe([this](const ContentBrowserEvent& event) {
            if (event.kind == ContentEventKind::SelectionChanged && event.primary.IsValid()) {
                m_History->PushRecent(event.primary);
            }
        });
    }

    [[nodiscard]] IAssetTreeModel& Model() noexcept override { return *m_Model; }
    [[nodiscard]] IAssetSelection& Selection() noexcept override { return *m_Selection; }
    [[nodiscard]] IAssetSearchProvider& Search() noexcept override { return *m_Search; }
    [[nodiscard]] IAssetCommandRouter& Commands() noexcept override { return *m_Commands; }
    [[nodiscard]] IContentBrowserEventRouter& Events() noexcept override { return *m_Events; }
    [[nodiscard]] IAssetDragDropHandler& DragDrop() noexcept override { return *m_DragDrop; }
    [[nodiscard]] IAssetThumbnailProvider& Thumbnails() noexcept override { return *m_Thumbnails; }
    [[nodiscard]] IAssetFolderProvider& Folders() noexcept override { return *m_Folders; }
    [[nodiscard]] IAssetCollectionProvider& Collections() noexcept override { return *m_Collections; }
    [[nodiscard]] IAssetHistory& History() noexcept override { return *m_History; }
    [[nodiscard]] IAssetClipboard& Clipboard() noexcept override { return *m_Clipboard; }
    [[nodiscard]] IAssetRegistryProvider& Registry() noexcept override { return *m_Registry; }

    void RegisterDataProvider(std::unique_ptr<IAssetDataProvider> provider) override {
        if (provider) {
            m_ExtraProviders.push_back(std::move(provider));
        }
    }
    void RegisterFilter(std::unique_ptr<IAssetFilterProvider> filter) override {
        if (filter) {
            m_Filters.push_back(std::move(filter));
        }
    }
    void RegisterContextMenu(std::unique_ptr<IAssetContextMenuProvider> provider) override {
        if (provider) {
            m_ContextMenus.push_back(std::move(provider));
        }
    }

    void Navigate(std::string_view virtualPath) override {
        m_Model->SetCurrentFolder(virtualPath);
        ContentBrowserService::Get().SetCurrentFolder(std::string(virtualPath));
        ContentBrowserEvent ev;
        ev.kind = ContentEventKind::FolderChanged;
        ev.detail = std::string(virtualPath);
        m_Events->Publish(ev);
        RequestRebuild();
    }

    [[nodiscard]] std::string_view GetCurrentFolder() const noexcept override {
        return m_Model->GetCurrentFolder();
    }

    void RequestRebuild() override { m_Model->MarkDirty(); }

    void Tick(float deltaSeconds) override {
        ContentBrowserService::Get().Tick(deltaSeconds);
        m_Registry->Tick(deltaSeconds);
        m_Thumbnails->Tick(deltaSeconds);
        if (m_Model->IsDirty()) {
            m_Model->Rebuild();
        }
    }

    void SetFilterState(const ContentFilterState& state) override {
        m_Model->SetFilterState(state);
        ContentBrowserEvent ev;
        ev.kind = ContentEventKind::FilterChanged;
        m_Events->Publish(ev);
    }

    [[nodiscard]] const ContentFilterState& GetFilterState() const noexcept override {
        return m_Model->GetFilterState();
    }

    void SaveFilterPreset(std::string_view name) override {
        ContentFilterPreset preset;
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

    [[nodiscard]] std::vector<ContentFilterPreset> ListFilterPresets() const override {
        std::vector<ContentFilterPreset> list;
        for (const auto& [_, preset] : m_Presets) {
            list.push_back(preset);
        }
        return list;
    }

    [[nodiscard]] bool Reveal(const ContentAssetId& id) override { return m_Commands->Reveal(id); }
    [[nodiscard]] bool RevealPath(std::string_view virtualPath) override {
        return m_Commands->RevealPath(virtualPath);
    }

    void Shutdown() {
        ContentBrowserService::Get().Shutdown();
    }

private:
    ContentBrowserDependencies m_Deps;
    std::unique_ptr<IContentBrowserEventRouter> m_Events;
    std::unique_ptr<IAssetSelection> m_Selection;
    std::unique_ptr<IAssetSearchProvider> m_Search;
    std::unique_ptr<IAssetTreeModel> m_Model;
    std::unique_ptr<IAssetSorter> m_Sorter;
    std::unique_ptr<IAssetRegistryProvider> m_Registry;
    std::unique_ptr<IAssetFolderProvider> m_Folders;
    std::unique_ptr<IAssetCollectionProvider> m_Collections;
    std::unique_ptr<IAssetHistory> m_History;
    std::unique_ptr<IAssetClipboard> m_Clipboard;
    std::unique_ptr<IAssetCommandRouter> m_Commands;
    std::unique_ptr<IAssetDragDropHandler> m_DragDrop;
    std::unique_ptr<IAssetThumbnailProvider> m_Thumbnails;
    std::vector<std::unique_ptr<IAssetFilterProvider>> m_Filters;
    std::vector<std::unique_ptr<IAssetDataProvider>> m_ExtraProviders;
    std::vector<std::unique_ptr<IAssetContextMenuProvider>> m_ContextMenus;
    std::unordered_map<std::string, ContentFilterPreset> m_Presets;
};

class ContentBrowserRuntimeImpl final : public IContentBrowserRuntime {
public:
    explicit ContentBrowserRuntimeImpl(ContentBrowserDependencies deps)
        : m_Browser(std::make_unique<ContentBrowserImpl>(std::move(deps)))
    {}

    [[nodiscard]] IContentBrowser& Browser() noexcept override { return *m_Browser; }
    [[nodiscard]] const IContentBrowser& Browser() const noexcept override { return *m_Browser; }

    void Shutdown() override {
        if (m_Browser) {
            m_Browser->Shutdown();
            m_Browser.reset();
        }
    }

private:
    std::unique_ptr<ContentBrowserImpl> m_Browser;
};

} // namespace detail

std::unique_ptr<IContentBrowserRuntime> CreateContentBrowserRuntime(const ContentBrowserDependencies& deps) {
    return std::make_unique<detail::ContentBrowserRuntimeImpl>(deps);
}

} // namespace we::editor::contentbrowser
