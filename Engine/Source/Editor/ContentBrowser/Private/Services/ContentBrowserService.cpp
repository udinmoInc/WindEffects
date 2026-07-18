#include "Services/ContentBrowserService.h"
#include "Services/ContentBrowserFolderArt.h"
#include "Services/ContentBrowserBlueprintArt.h"
#include "Registry/AssetTypeResolver.h"
#include "KindUI/Rendering/IconRenderer.h"
#include "Core/Logger.h"
#include "Core/Paths.h"
#include "KindUI/Core/UIRepaintGate.h"
#include <filesystem>

namespace we::editor::contentbrowser {
using ::we::runtime::kindui::UIRepaintGate;
namespace Icons = ::we::runtime::kindui::Icons;


ContentBrowserService& ContentBrowserService::Get() {
    static ContentBrowserService instance;
    return instance;
}

void ContentBrowserService::Initialize(we::runtime::kindui::IconRenderer* iconRenderer, const std::string& contentRoot) {
    if (m_Initialized) return;

    m_IconRenderer = iconRenderer;
    ContentBrowserFolderArt::Get().Initialize(iconRenderer);
    ContentBrowserBlueprintArt::Get().Initialize(iconRenderer);
    m_DiskCache.SetCacheDirectory(we::core::PathService::Get().ThumbnailCacheRoot());
    m_ThumbnailManager.SetDiskCache(&m_DiskCache);
    m_ThumbnailManager.SetFolderPreviewGenerator(&m_FolderPreview);

    ContentAssetRegistry::Get().Initialize(contentRoot);
    ContentAssetRegistry::Get().SetOnRegistryRefreshed([this]() { OnRegistryRefreshed(); });

    m_Initialized = true;
    HE_INFO("[ContentBrowserService] Initialized");
}

void ContentBrowserService::Shutdown() {
    if (!m_Initialized) return;
    ContentAssetRegistry::Get().Shutdown();
    m_ThumbnailManager.InvalidateAll();
    m_FolderPreview.InvalidateAll();
    ContentBrowserFolderArt::Get().InvalidateCache();
    ContentBrowserBlueprintArt::Get().InvalidateCache();
    m_Initialized = false;
}

void ContentBrowserService::Tick(float deltaTime) {
    if (!m_Initialized) return;
    ContentAssetRegistry::Get().Tick(deltaTime);
    ProcessThumbnails();
}

void ContentBrowserService::SetCurrentFolder(const std::string& virtualPath) {
    m_CurrentFolder = virtualPath.empty() ? "/Game" : virtualPath;
    if (auto model = m_Model.lock()) {
        RefreshBrowserModel(model);
    }
}

void ContentBrowserService::RefreshBrowserModel(const std::shared_ptr<::we::editor::contentbrowser::ContentBrowserModel>& model) {
    if (!model) return;
    m_Model = model;
    model->items.clear();

    const auto children = ContentAssetRegistry::Get().GetChildren(m_CurrentFolder);
    for (const auto* asset : children) {
        ::we::editor::contentbrowser::ContentItem item;
        item.id = asset->id;
        item.name = asset->name;
        item.type = AssetTypeToString(asset->type);
        item.path = asset->virtualPath;
        item.isFolder = asset->isFolder;
        item.isFavorite = asset->isFavorite;
        item.iconName = asset->isFolder ? we::runtime::kindui::Icons::FolderName : AssetTypeToKey(asset->type);
        if (!asset->isFolder) {
            item.iconTexture = m_ThumbnailManager.GetCachedTexture(asset->id);
            item.thumbnailRequested = item.iconTexture != we::rhi::RHIDescriptorSetHandle::Invalid;
        }
        item.userData = const_cast<AssetRecord*>(asset);
        model->items.push_back(item);
    }

    model->folderCount = 0;
    model->assetCount = 0;
    for (const auto& item : model->items) {
        if (item.isFolder) ++model->folderCount;
        else ++model->assetCount;
    }

    model->NotifyChanged();
}

void ContentBrowserService::RequestThumbnailForItem(const std::string& id) {
    const auto* asset = ContentAssetRegistry::Get().FindById(id);
    if (!asset || asset->isFolder) return;

    ThumbnailRequest request;
    request.id = asset->id;
    request.type = AssetTypeToKey(asset->type);
    request.path = asset->virtualPath;
    request.isFolder = false;
    request.sourceVersion = asset->modifiedTime;
    request.folderVersion = ContentAssetRegistry::Get().GetFolderContentVersion(asset->virtualPath);
    m_ThumbnailManager.RequestThumbnail(request);
}

void ContentBrowserService::SetVisibleItemIds(const std::unordered_set<std::string>& ids) {
    m_ThumbnailManager.SetVisibleItems(ids);
}

we::rhi::RHIDescriptorSetHandle ContentBrowserService::UploadBitmap(const BitmapRGBA& bitmap) {
    if (!m_IconRenderer || bitmap.pixels.empty()) return we::rhi::RHIDescriptorSetHandle::Invalid;
    return m_IconRenderer->CreateTextureFromBitmap(bitmap.pixels, bitmap.width, bitmap.height);
}

void ContentBrowserService::ProcessThumbnails() {
    m_ThumbnailManager.ProcessCompletedRequests(
        [this](const BitmapRGBA& bitmap) { return UploadBitmap(bitmap); },
        [this](const std::string& id, we::rhi::RHIDescriptorSetHandle texture) {
            if (m_OnThumbnailReady) m_OnThumbnailReady(id, texture);
            if (auto model = m_Model.lock()) {
                for (auto& item : model->items) {
                    if (item.id == id) {
                        item.iconTexture = texture;
                        break;
                    }
                }
                model->NotifyChanged();
            }
            we::runtime::kindui::UIRepaintGate::Request();
        });
}

void ContentBrowserService::OnRegistryRefreshed() {
    m_FolderPreview.InvalidateAll();
    m_ThumbnailManager.InvalidateAll();
    if (auto model = m_Model.lock()) {
        RefreshBrowserModel(model);
    }
}

size_t ContentBrowserService::GetMemoryUsageBytes() const {
    return m_ThumbnailManager.HasCachedTexture("") ? 0 : 0;
}

} // namespace we::editor::contentbrowser
