#pragma once

#include "Controllers/FilterController.hpp"
#include "Controllers/SearchController.hpp"
#include "Registry/ContentAssetRegistry.hpp"
#include "Services/DiskThumbnailCache.hpp"
#include "Services/FolderPreviewGenerator.hpp"
#include "Services/ThumbnailManager.hpp"
#include "Models/ContentBrowserModel.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <volk.h>

namespace we::UI {
class IconRenderer;
}

namespace we::editor::contentbrowser {

class ContentBrowserService {
public:
    static ContentBrowserService& Get();

    void Initialize(we::UI::IconRenderer* iconRenderer, const std::string& contentRoot);
    void Shutdown();
    void Tick(float deltaTime);

    ContentAssetRegistry& GetRegistry() { return ContentAssetRegistry::Get(); }
    ThumbnailManager& GetThumbnailManager() { return m_ThumbnailManager; }
    SearchController& GetSearchController() { return m_SearchController; }
    FilterController& GetFilterController() { return m_FilterController; }

    void SetCurrentFolder(const std::string& virtualPath);
    const std::string& GetCurrentFolder() const { return m_CurrentFolder; }

    void RefreshBrowserModel(const std::shared_ptr<we::UI::ContentBrowserModel>& model);
    void RequestThumbnailForItem(const std::string& id);
    void SetVisibleItemIds(const std::unordered_set<std::string>& ids);

    VkDescriptorSet GetFolderThumbnailTexture() const { return m_FolderThumbnailTexture; }

    void SetOnThumbnailReady(std::function<void(const std::string&, VkDescriptorSet)> callback) {
        m_OnThumbnailReady = std::move(callback);
    }

    size_t GetMemoryUsageBytes() const;

private:
    void OnRegistryRefreshed();
    void ProcessThumbnails();
    void EnsureFolderThumbnail();
    VkDescriptorSet UploadBitmap(const struct BitmapRGBA& bitmap);

    we::UI::IconRenderer* m_IconRenderer = nullptr;
    VkDescriptorSet m_FolderThumbnailTexture = VK_NULL_HANDLE;
    ThumbnailManager m_ThumbnailManager;
    DiskThumbnailCache m_DiskCache;
    FolderPreviewGenerator m_FolderPreview;
    SearchController m_SearchController;
    FilterController m_FilterController;

    std::string m_CurrentFolder = "/Game";
    std::function<void(const std::string&, VkDescriptorSet)> m_OnThumbnailReady;
    std::weak_ptr<we::UI::ContentBrowserModel> m_Model;
    bool m_Initialized = false;
};

} // namespace we::editor::contentbrowser
