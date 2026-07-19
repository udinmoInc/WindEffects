#pragma once

#include "ContentBrowser/Export.h"
#include "ContentBrowser/ContentBrowserTypes.h"
#include "ContentBrowser/IContentAsset.h"
#include "ContentBrowser/IAssetServices.h"
#include "ContentBrowser/IAssetCommands.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::kindui {
class IconRenderer;
}

namespace we::editor::contentbrowser {

class CONTENTBROWSER_API IContentBrowser {
public:
    virtual ~IContentBrowser() = default;

    [[nodiscard]] virtual IAssetTreeModel& Model() noexcept = 0;
    [[nodiscard]] virtual IAssetSelection& Selection() noexcept = 0;
    [[nodiscard]] virtual IAssetSearchProvider& Search() noexcept = 0;
    [[nodiscard]] virtual IAssetCommandRouter& Commands() noexcept = 0;
    [[nodiscard]] virtual IContentBrowserEventRouter& Events() noexcept = 0;
    [[nodiscard]] virtual IAssetDragDropHandler& DragDrop() noexcept = 0;
    [[nodiscard]] virtual IAssetThumbnailProvider& Thumbnails() noexcept = 0;
    [[nodiscard]] virtual IAssetFolderProvider& Folders() noexcept = 0;
    [[nodiscard]] virtual IAssetCollectionProvider& Collections() noexcept = 0;
    [[nodiscard]] virtual IAssetHistory& History() noexcept = 0;
    [[nodiscard]] virtual IAssetClipboard& Clipboard() noexcept = 0;
    [[nodiscard]] virtual IAssetRegistryProvider& Registry() noexcept = 0;

    virtual void RegisterDataProvider(std::unique_ptr<IAssetDataProvider> provider) = 0;
    virtual void RegisterFilter(std::unique_ptr<IAssetFilterProvider> filter) = 0;
    virtual void RegisterContextMenu(std::unique_ptr<IAssetContextMenuProvider> provider) = 0;

    virtual void Navigate(std::string_view virtualPath) = 0;
    [[nodiscard]] virtual std::string_view GetCurrentFolder() const noexcept = 0;
    virtual void RequestRebuild() = 0;
    virtual void Tick(float deltaSeconds) = 0;

    virtual void SetFilterState(const ContentFilterState& state) = 0;
    [[nodiscard]] virtual const ContentFilterState& GetFilterState() const noexcept = 0;
    virtual void SaveFilterPreset(std::string_view name) = 0;
    [[nodiscard]] virtual bool LoadFilterPreset(std::string_view name) = 0;
    [[nodiscard]] virtual std::vector<ContentFilterPreset> ListFilterPresets() const = 0;

    [[nodiscard]] virtual bool Reveal(const ContentAssetId& id) = 0;
    [[nodiscard]] virtual bool RevealPath(std::string_view virtualPath) = 0;
};

class CONTENTBROWSER_API IContentBrowserRuntime {
public:
    virtual ~IContentBrowserRuntime() = default;
    [[nodiscard]] virtual IContentBrowser& Browser() noexcept = 0;
    [[nodiscard]] virtual const IContentBrowser& Browser() const noexcept = 0;
    virtual void Shutdown() = 0;
};

} // namespace we::editor::contentbrowser
