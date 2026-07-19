#pragma once

#include "ContentBrowser/Export.h"
#include "ContentBrowser/ContentBrowserTypes.h"
#include "ContentBrowser/IContentAsset.h"
#include "RHI/Types.h"

#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace we::editor::contentbrowser {

class CONTENTBROWSER_API IAssetThumbnailProvider {
public:
    virtual ~IAssetThumbnailProvider() = default;
    virtual void Request(const ContentAssetId& id) = 0;
    [[nodiscard]] virtual we::rhi::RHIDescriptorSetHandle GetCached(const ContentAssetId& id) const = 0;
    virtual void SetVisible(std::span<const ContentAssetId> ids) = 0;
    virtual void Invalidate(const ContentAssetId& id) = 0;
    virtual void Tick(float deltaSeconds) = 0;
};

class CONTENTBROWSER_API IAssetPreviewProvider {
public:
    virtual ~IAssetPreviewProvider() = default;
    [[nodiscard]] virtual bool CanPreview(const ContentAssetId& id) const = 0;
    virtual void ShowPreview(const ContentAssetId& id) = 0;
};

class CONTENTBROWSER_API IAssetFolderProvider {
public:
    virtual ~IAssetFolderProvider() = default;
    [[nodiscard]] virtual bool CreateFolder(std::string_view parentVirtualPath, std::string_view name) = 0;
    [[nodiscard]] virtual bool RenameFolder(const ContentAssetId& id, std::string_view newName) = 0;
    [[nodiscard]] virtual bool MoveFolder(const ContentAssetId& id, std::string_view newParentVirtualPath) = 0;
    [[nodiscard]] virtual bool DeleteFolder(const ContentAssetId& id) = 0;
};

class CONTENTBROWSER_API IAssetCollectionProvider {
public:
    virtual ~IAssetCollectionProvider() = default;
    [[nodiscard]] virtual std::vector<std::string> ListCollections() const = 0;
    [[nodiscard]] virtual bool CreateCollection(std::string_view name) = 0;
    [[nodiscard]] virtual bool AddToCollection(std::string_view name, const ContentAssetId& id) = 0;
    [[nodiscard]] virtual bool RemoveFromCollection(std::string_view name, const ContentAssetId& id) = 0;
    [[nodiscard]] virtual std::vector<ContentAssetId> GetCollectionAssets(std::string_view name) const = 0;
};

class CONTENTBROWSER_API IAssetTagProvider {
public:
    virtual ~IAssetTagProvider() = default;
    [[nodiscard]] virtual std::vector<std::string> GetTags(const ContentAssetId& id) const = 0;
    [[nodiscard]] virtual bool AddTag(const ContentAssetId& id, std::string_view tag) = 0;
    [[nodiscard]] virtual bool RemoveTag(const ContentAssetId& id, std::string_view tag) = 0;
};

class CONTENTBROWSER_API IAssetMetadataProvider {
public:
    virtual ~IAssetMetadataProvider() = default;
    [[nodiscard]] virtual std::string GetDisplayType(const ContentAssetId& id) const = 0;
    [[nodiscard]] virtual std::string GetGuidString(const ContentAssetId& id) const = 0;
};

class CONTENTBROWSER_API IAssetDependencyProvider {
public:
    virtual ~IAssetDependencyProvider() = default;
    [[nodiscard]] virtual std::vector<ContentAssetId> GetDependencies(const ContentAssetId& id) const = 0;
    [[nodiscard]] virtual std::vector<ContentAssetId> GetReferencers(const ContentAssetId& id) const = 0;
};

class CONTENTBROWSER_API IAssetReferenceProvider {
public:
    virtual ~IAssetReferenceProvider() = default;
    [[nodiscard]] virtual bool HasHardReferences(const ContentAssetId& id) const = 0;
};

class CONTENTBROWSER_API IAssetHistory {
public:
    virtual ~IAssetHistory() = default;
    virtual void PushRecent(const ContentAssetId& id) = 0;
    [[nodiscard]] virtual std::span<const ContentAssetId> GetRecent() const noexcept = 0;
    virtual void AddBookmark(std::string_view name, std::string_view virtualPath) = 0;
    [[nodiscard]] virtual std::vector<std::pair<std::string, std::string>> GetBookmarks() const = 0;
};

class CONTENTBROWSER_API IAssetClipboard {
public:
    virtual ~IAssetClipboard() = default;
    virtual void Copy(std::span<const ContentAssetId> ids) = 0;
    virtual void Cut(std::span<const ContentAssetId> ids) = 0;
    [[nodiscard]] virtual bool CanPaste() const noexcept = 0;
    [[nodiscard]] virtual bool Paste(std::string_view targetFolderVirtualPath) = 0;
};

class CONTENTBROWSER_API IAssetValidator {
public:
    virtual ~IAssetValidator() = default;
    [[nodiscard]] virtual bool Validate(const ContentAssetId& id, std::string& outMessage) const = 0;
};

} // namespace we::editor::contentbrowser
