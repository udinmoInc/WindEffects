#include "Registry/ContentAssetRegistry.h"
#include "Registry/AssetTypeResolver.h"
#include "Core/Logger.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace we::editor::contentbrowser {

ContentAssetRegistry& ContentAssetRegistry::Get() {
    static ContentAssetRegistry instance;
    return instance;
}

void ContentAssetRegistry::Initialize(const std::string& contentRoot) {
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_ContentRoot = contentRoot;
    }

    // Never seed demo/sample content. Empty projects stay empty until the user adds assets.

    // Refresh acquires m_Mutex internally; do not hold the lock here (std::mutex is not recursive).
    Refresh();

    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Initialized = true;
    }

    HE_INFO("[ContentAssetRegistry] Initialized at: " + contentRoot);
}

void ContentAssetRegistry::Shutdown() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Assets.clear();
    m_IdIndex.clear();
    m_PathIndex.clear();
    m_FolderVersions.clear();
    m_Initialized = false;
}

std::string ContentAssetRegistry::MakeId(const std::string& virtualPath) const {
    return virtualPath;
}

void ContentAssetRegistry::Refresh() {
    std::function<void()> refreshedCallback;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Assets.clear();
        m_IdIndex.clear();
        m_PathIndex.clear();
        m_FolderVersions.clear();

        // Project Content/ mounts as virtual /Game (Unreal-style). Never hardcode a project path.
        const fs::path contentRoot = fs::path(m_ContentRoot);
        if (!fs::exists(contentRoot)) {
            refreshedCallback = m_OnRegistryRefreshed;
        } else {
            ScanDirectory(contentRoot.string(), "/Game");

            for (auto& asset : m_Assets) {
                if (asset.isFolder) {
                    uint32_t version = 0;
                    for (const auto& child : m_Assets) {
                        if (child.parentPath == asset.virtualPath) {
                            version += child.contentVersion + 1;
                        }
                    }
                    m_FolderVersions[asset.virtualPath] = version;
                    asset.contentVersion = version;
                } else {
                    asset.contentVersion = static_cast<uint32_t>(asset.modifiedTime & 0xFFFFFFFFu);
                }
            }

            refreshedCallback = m_OnRegistryRefreshed;
        }
    }

    if (refreshedCallback) {
        refreshedCallback();
    }
}

void ContentAssetRegistry::ScanDirectory(const std::string& diskPath, const std::string& virtualPath) {
    std::error_code ec;

    AssetRecord folder{};
    folder.id = MakeId(virtualPath);
    folder.name = virtualPath == "/Game" ? "Game" : fs::path(virtualPath).filename().string();
    folder.virtualPath = virtualPath;
    folder.diskPath = diskPath;
    folder.parentPath = virtualPath == "/Game" ? "" : fs::path(virtualPath).parent_path().string();
    folder.type = AssetType::Folder;
    folder.isFolder = true;
    folder.extension = "";
    if (fs::exists(diskPath, ec)) {
        auto ftime = fs::last_write_time(diskPath, ec);
        folder.modifiedTime = static_cast<uint64_t>(ftime.time_since_epoch().count());
    }
    m_IdIndex[folder.id] = m_Assets.size();
    m_PathIndex[folder.virtualPath] = m_Assets.size();
    m_Assets.push_back(folder);

    for (const auto& entry : fs::directory_iterator(diskPath, ec)) {
        if (ec) break;

        const std::string entryName = entry.path().filename().string();
        if (!entryName.empty() && entryName[0] == '.') continue;

        const std::string childVirtual = virtualPath + "/" + entryName;
        const std::string childDisk = entry.path().string();

        if (entry.is_directory(ec)) {
            ScanDirectory(childDisk, childVirtual);
            continue;
        }

        AssetRecord asset{};
        asset.id = MakeId(childVirtual);
        asset.name = entryName;
        asset.virtualPath = childVirtual;
        asset.diskPath = childDisk;
        asset.parentPath = virtualPath;
        asset.isFolder = false;
        asset.extension = entry.path().extension().string();
        asset.type = AssetTypeResolver::FromPath(childDisk, false);
        asset.fileSize = entry.file_size(ec);
        auto ftime = fs::last_write_time(entry, ec);
        asset.modifiedTime = static_cast<uint64_t>(ftime.time_since_epoch().count());
        asset.contentVersion = static_cast<uint32_t>(asset.modifiedTime & 0xFFFFFFFFu);

        m_IdIndex[asset.id] = m_Assets.size();
        m_PathIndex[asset.virtualPath] = m_Assets.size();
        m_Assets.push_back(asset);
    }
}

void ContentAssetRegistry::Tick(float deltaTime) {
    bool initialized = false;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        initialized = m_Initialized;
    }
    if (!initialized) return;
    m_WatchTimer += deltaTime;
    if (m_WatchTimer < m_WatchInterval) return;
    m_WatchTimer = 0.0f;

    const fs::path gameRoot = fs::path(m_ContentRoot) / "Game";
    if (!fs::exists(gameRoot)) return;

    static uint64_t lastScanSignature = 0;
    uint64_t signature = 0;
    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(gameRoot, ec)) {
        signature += static_cast<uint64_t>(entry.file_size(ec));
        auto ftime = fs::last_write_time(entry, ec);
        signature ^= static_cast<uint64_t>(ftime.time_since_epoch().count());
    }

    if (signature != lastScanSignature) {
        lastScanSignature = signature;
        Refresh();
    }
}

const AssetRecord* ContentAssetRegistry::FindById(const std::string& id) const {
    auto it = m_IdIndex.find(id);
    if (it == m_IdIndex.end()) return nullptr;
    return &m_Assets[it->second];
}

const AssetRecord* ContentAssetRegistry::FindByVirtualPath(const std::string& virtualPath) const {
    auto it = m_PathIndex.find(virtualPath);
    if (it == m_PathIndex.end()) return nullptr;
    return &m_Assets[it->second];
}

std::vector<const AssetRecord*> ContentAssetRegistry::GetChildren(const std::string& folderVirtualPath) const {
    std::vector<const AssetRecord*> children;
    for (const auto& asset : m_Assets) {
        if (asset.parentPath == folderVirtualPath) {
            children.push_back(&asset);
        }
    }
    std::sort(children.begin(), children.end(), [](const AssetRecord* a, const AssetRecord* b) {
        if (a->isFolder != b->isFolder) return a->isFolder > b->isFolder;
        return a->name < b->name;
    });
    return children;
}

std::vector<const AssetRecord*> ContentAssetRegistry::GetFolderContents(
    const std::string& folderVirtualPath, bool recursive) const
{
    std::vector<const AssetRecord*> result;
    std::function<void(const std::string&)> walk = [&](const std::string& path) {
        for (const auto& asset : m_Assets) {
            if (asset.parentPath == path) {
                if (!asset.isFolder) {
                    result.push_back(&asset);
                } else if (recursive) {
                    walk(asset.virtualPath);
                }
            }
        }
    };
    walk(folderVirtualPath);
    return result;
}

void ContentAssetRegistry::ToggleFavorite(const std::string& id) {
    auto it = m_IdIndex.find(id);
    if (it == m_IdIndex.end()) return;
    m_Assets[it->second].isFavorite = !m_Assets[it->second].isFavorite;
    if (m_OnAssetChanged) m_OnAssetChanged(m_Assets[it->second]);
}

bool ContentAssetRegistry::IsFavorite(const std::string& id) const {
    auto it = m_IdIndex.find(id);
    if (it == m_IdIndex.end()) return false;
    return m_Assets[it->second].isFavorite;
}

uint32_t ContentAssetRegistry::GetFolderContentVersion(const std::string& folderVirtualPath) const {
    auto it = m_FolderVersions.find(folderVirtualPath);
    if (it == m_FolderVersions.end()) return 0;
    return it->second;
}

void ContentAssetRegistry::NotifyRegistryRefreshed() {
    std::function<void()> refreshedCallback;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        refreshedCallback = m_OnRegistryRefreshed;
    }
    if (refreshedCallback) {
        refreshedCallback();
    }
}

} // namespace we::editor::contentbrowser
