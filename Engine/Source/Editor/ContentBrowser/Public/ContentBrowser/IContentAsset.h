#pragma once

#include "ContentBrowser/Export.h"
#include "ContentBrowser/ContentBrowserTypes.h"
#include "RHI/Types.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::editor::contentbrowser {

class CONTENTBROWSER_API IContentAssetNode {
public:
    virtual ~IContentAssetNode() = default;

    [[nodiscard]] virtual ContentAssetId GetId() const = 0;
    [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetVirtualPath() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetDiskPath() const noexcept = 0;
    [[nodiscard]] virtual ContentAssetKind GetKind() const noexcept = 0;
    [[nodiscard]] virtual bool IsFolder() const noexcept = 0;
    [[nodiscard]] virtual bool IsFavorite() const noexcept = 0;
    [[nodiscard]] virtual std::span<const std::string> GetTags() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t GetModifiedTime() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t GetFileSize() const noexcept = 0;
};

class CONTENTBROWSER_API IAssetDataProvider {
public:
    virtual ~IAssetDataProvider() = default;
    [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;
    [[nodiscard]] virtual std::vector<ContentAssetId> GetRootFolders() const = 0;
    [[nodiscard]] virtual std::vector<ContentAssetId> GetChildren(const ContentAssetId& folder) const = 0;
    [[nodiscard]] virtual const IContentAssetNode* Find(const ContentAssetId& id) const = 0;
    [[nodiscard]] virtual const IContentAssetNode* FindByPath(std::string_view virtualPath) const = 0;
    virtual void Rebuild() = 0;
};

class CONTENTBROWSER_API IAssetRegistryProvider {
public:
    virtual ~IAssetRegistryProvider() = default;
    virtual void Refresh() = 0;
    virtual void Tick(float deltaSeconds) = 0;
    [[nodiscard]] virtual std::size_t GetAssetCount() const noexcept = 0;
    [[nodiscard]] virtual IAssetDataProvider& Data() noexcept = 0;
};

class CONTENTBROWSER_API IAssetTreeModel {
public:
    virtual ~IAssetTreeModel() = default;
    virtual void Rebuild() = 0;
    virtual void MarkDirty() = 0;
    [[nodiscard]] virtual bool IsDirty() const noexcept = 0;
    [[nodiscard]] virtual std::span<const ContentAssetId> GetVisibleAssets() const noexcept = 0;
    [[nodiscard]] virtual const IContentAssetNode* GetNode(const ContentAssetId& id) const = 0;
    virtual void SetFilterState(const ContentFilterState& state) = 0;
    [[nodiscard]] virtual const ContentFilterState& GetFilterState() const noexcept = 0;
    virtual void SetCurrentFolder(std::string_view virtualPath) = 0;
    [[nodiscard]] virtual std::string_view GetCurrentFolder() const noexcept = 0;
};

class CONTENTBROWSER_API IAssetSelection {
public:
    virtual ~IAssetSelection() = default;
    [[nodiscard]] virtual std::span<const ContentAssetId> GetSelected() const noexcept = 0;
    [[nodiscard]] virtual ContentAssetId GetPrimary() const = 0;
    [[nodiscard]] virtual bool IsSelected(const ContentAssetId& id) const noexcept = 0;
    [[nodiscard]] virtual bool IsEmpty() const noexcept = 0;
    virtual void Clear() = 0;
    virtual void Set(const ContentAssetId& id) = 0;
    virtual void SetMany(std::span<const ContentAssetId> ids) = 0;
    virtual void Apply(ContentSelectionOp op, const ContentAssetId& id) = 0;
};

class CONTENTBROWSER_API IAssetSearchProvider {
public:
    virtual ~IAssetSearchProvider() = default;
    virtual void SetQuery(std::string_view query) = 0;
    [[nodiscard]] virtual std::string_view GetQuery() const noexcept = 0;
    [[nodiscard]] virtual bool Matches(const IContentAssetNode& node) const = 0;
};

class CONTENTBROWSER_API IAssetFilterProvider {
public:
    virtual ~IAssetFilterProvider() = default;
    [[nodiscard]] virtual bool Passes(const IContentAssetNode& node, const ContentFilterState& state) const = 0;
};

class CONTENTBROWSER_API IAssetSorter {
public:
    virtual ~IAssetSorter() = default;
    virtual void Sort(std::vector<ContentAssetId>& ids, ContentSortMode mode) const = 0;
};

} // namespace we::editor::contentbrowser
