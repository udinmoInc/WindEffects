#pragma once

#include "Controllers/FilterController.h"
#include "Controllers/SearchController.h"
#include "Registry/ContentAssetRegistry.h"
#include "Services/DiskThumbnailCache.h"
#include "Services/FolderPreviewGenerator.h"
#include "Services/ThumbnailManager.h"
#include "ContentBrowser/Models/ContentBrowserModel.h"
#include "KindUI/Rendering/IconRenderer.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include "RHI/Types.h"

namespace we::editor::contentbrowser {
using ::we::runtime::kindui::IconRenderer;

class ContentBrowserService {
public:
    static ContentBrowserService& Get();

    void Initialize(IconRenderer* iconRenderer, const std::string& contentRoot);
    void Shutdown();
    void Tick(float deltaTime);

    ContentAssetRegistry& GetRegistry() { return ContentAssetRegistry::Get(); }
    ThumbnailManager& GetThumbnailManager() { return m_ThumbnailManager; }
    SearchController& GetSearchController() { return m_SearchController; }
    FilterController& GetFilterController() { return m_FilterController; }

    void SetCurrentFolder(const std::string& virtualPath);
    const std::string& GetCurrentFolder() const { return m_CurrentFolder; }

    void RefreshBrowserModel(const std::shared_ptr<::we::editor::contentbrowser::ContentBrowserModel>& model);
    void RequestThumbnailForItem(const std::string& id);
    void SetVisibleItemIds(const std::unordered_set<std::string>& ids);

    void SetOnThumbnailReady(std::function<void(const std::string&, we::rhi::RHIDescriptorSetHandle)> callback) {
        m_OnThumbnailReady = std::move(callback);
    }

    size_t GetMemoryUsageBytes() const;

private:
    void OnRegistryRefreshed();
    void ProcessThumbnails();
    we::rhi::RHIDescriptorSetHandle UploadBitmap(const struct BitmapRGBA& bitmap);

    we::runtime::kindui::IconRenderer* m_IconRenderer = nullptr;
    ThumbnailManager m_ThumbnailManager;
    DiskThumbnailCache m_DiskCache;
    FolderPreviewGenerator m_FolderPreview;
    SearchController m_SearchController;
    FilterController m_FilterController;

    std::string m_CurrentFolder = "/Game";
    std::function<void(const std::string&, we::rhi::RHIDescriptorSetHandle)> m_OnThumbnailReady;
    std::weak_ptr<::we::editor::contentbrowser::ContentBrowserModel> m_Model;
    bool m_Initialized = false;
};

} // namespace we::editor::contentbrowser
