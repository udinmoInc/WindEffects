#pragma once

#include "ContentBrowser/Export.h"
#include "ContentBrowser/ContentBrowserTypes.h"
#include "ContentBrowser/IContentAsset.h"

#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::editor::contentbrowser {

struct CONTENTBROWSER_API ContentBrowserEvent {
    ContentEventKind kind = ContentEventKind::SelectionChanged;
    std::vector<ContentAssetId> assets;
    ContentAssetId primary{};
    std::string detail;
};

using ContentBrowserEventListener = std::function<void(const ContentBrowserEvent&)>;

class CONTENTBROWSER_API IContentBrowserEventRouter {
public:
    virtual ~IContentBrowserEventRouter() = default;
    virtual void Publish(const ContentBrowserEvent& event) = 0;
    virtual void Subscribe(ContentBrowserEventListener listener) = 0;
    virtual void Suspend(bool suspend) = 0;
};

class CONTENTBROWSER_API IAssetCommandRouter {
public:
    virtual ~IAssetCommandRouter() = default;

    [[nodiscard]] virtual bool Rename(const ContentAssetId& id, std::string_view newName) = 0;
    [[nodiscard]] virtual bool Move(const ContentAssetId& id, std::string_view newParentVirtualPath) = 0;
    [[nodiscard]] virtual bool Duplicate(const ContentAssetId& id) = 0;
    [[nodiscard]] virtual bool DeleteSelected() = 0;
    [[nodiscard]] virtual bool CopySelected() = 0;
    [[nodiscard]] virtual bool Paste(std::string_view targetFolder) = 0;
    [[nodiscard]] virtual bool Import(std::string_view sourcePath, std::string_view targetFolder) = 0;
    [[nodiscard]] virtual bool Reimport(const ContentAssetId& id) = 0;
    [[nodiscard]] virtual bool Export(const ContentAssetId& id, std::string_view destination) = 0;
    [[nodiscard]] virtual bool ValidateSelected() = 0;
    [[nodiscard]] virtual bool OpenInEditor(const ContentAssetId& id) = 0;
    [[nodiscard]] virtual bool RevealInOS(const ContentAssetId& id) = 0;
    [[nodiscard]] virtual bool Reveal(const ContentAssetId& id) = 0;
    [[nodiscard]] virtual bool RevealPath(std::string_view virtualPath) = 0;
    [[nodiscard]] virtual bool ToggleFavorite(const ContentAssetId& id) = 0;
    [[nodiscard]] virtual bool CreateFolder(std::string_view name) = 0;
    [[nodiscard]] virtual bool Refresh() = 0;
};

class CONTENTBROWSER_API IAssetContextMenuProvider {
public:
    virtual ~IAssetContextMenuProvider() = default;

    struct MenuItem {
        std::string id;
        std::string label;
        bool enabled = true;
    };

    [[nodiscard]] virtual std::vector<MenuItem> BuildMenu(std::span<const ContentAssetId> selection) const = 0;
    [[nodiscard]] virtual bool Execute(std::string_view itemId, std::span<const ContentAssetId> selection) = 0;
};

class CONTENTBROWSER_API IAssetDragDropHandler {
public:
    virtual ~IAssetDragDropHandler() = default;
    [[nodiscard]] virtual bool CanDrop(const ContentAssetId& dragged, std::string_view targetFolder) const = 0;
    [[nodiscard]] virtual bool Drop(const ContentAssetId& dragged, std::string_view targetFolder) = 0;
    [[nodiscard]] virtual bool DropExternal(std::string_view sourcePath, std::string_view targetFolder) = 0;
};

class CONTENTBROWSER_API IAssetImportHandler {
public:
    virtual ~IAssetImportHandler() = default;
    [[nodiscard]] virtual bool ImportFile(std::string_view sourcePath, std::string_view targetFolder) = 0;
    [[nodiscard]] virtual bool Reimport(const ContentAssetId& id) = 0;
};

} // namespace we::editor::contentbrowser
