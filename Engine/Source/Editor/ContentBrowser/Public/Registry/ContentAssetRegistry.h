#pragma once

#include "Registry/AssetTypes.h"
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::editor::contentbrowser {

class ContentAssetRegistry {
public:
    using AssetChangedCallback = std::function<void(const AssetRecord&)>;

    static ContentAssetRegistry& Get();

    void Initialize(const std::string& contentRoot);
    void Shutdown();
    void Refresh();
    void Tick(float deltaTime);

    const std::vector<AssetRecord>& GetAllAssets() const { return m_Assets; }
    const AssetRecord* FindById(const std::string& id) const;
    const AssetRecord* FindByVirtualPath(const std::string& virtualPath) const;

    std::vector<const AssetRecord*> GetChildren(const std::string& folderVirtualPath) const;
    std::vector<const AssetRecord*> GetFolderContents(const std::string& folderVirtualPath, bool recursive) const;

    std::string GetContentRoot() const { return m_ContentRoot; }
    std::string GetVirtualRoot() const { return "/Game"; }

    void SetOnAssetAdded(AssetChangedCallback cb) { m_OnAssetAdded = std::move(cb); }
    void SetOnAssetRemoved(AssetChangedCallback cb) { m_OnAssetRemoved = std::move(cb); }
    void SetOnAssetChanged(AssetChangedCallback cb) { m_OnAssetChanged = std::move(cb); }
    void SetOnRegistryRefreshed(std::function<void()> cb) { m_OnRegistryRefreshed = std::move(cb); }

    void ToggleFavorite(const std::string& id);
    bool IsFavorite(const std::string& id) const;

    uint32_t GetFolderContentVersion(const std::string& folderVirtualPath) const;

private:
    void ScanDirectory(const std::string& diskPath, const std::string& virtualPath);
    void EnsureDemoContent();
    std::string MakeId(const std::string& virtualPath) const;
    void NotifyRegistryRefreshed();

    std::string m_ContentRoot;
    std::vector<AssetRecord> m_Assets;
    std::unordered_map<std::string, size_t> m_IdIndex;
    std::unordered_map<std::string, size_t> m_PathIndex;
    std::unordered_map<std::string, uint32_t> m_FolderVersions;

    AssetChangedCallback m_OnAssetAdded;
    AssetChangedCallback m_OnAssetRemoved;
    AssetChangedCallback m_OnAssetChanged;
    std::function<void()> m_OnRegistryRefreshed;

    float m_WatchTimer = 0.0f;
    float m_WatchInterval = 2.0f;
    bool m_Initialized = false;
    mutable std::mutex m_Mutex;
};

} // namespace we::editor::contentbrowser
