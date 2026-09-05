#include "Services/ContentBrowserFolderArt.h"

#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"

#include <algorithm>

namespace we::editor::contentbrowser {
namespace kindui = ::we::runtime::kindui;
namespace WindIcons = ::we::runtime::kindui::WindIcons;

namespace {

kindui::Color FolderTint(bool hovered) {
    kindui::Color tint = kindui::ResolveColor(kindui::ColorToken::ContentBrowserFolderPrimary);
    if (hovered) {
        tint = kindui::Color::Pick(
            tint,
            kindui::ResolveColor(kindui::ColorToken::ContentBrowserFolderTab),
            0.4f);
    }
    return tint;
}

} // namespace

ContentBrowserFolderArt& ContentBrowserFolderArt::Get() {
    static ContentBrowserFolderArt instance;
    return instance;
}

void ContentBrowserFolderArt::Initialize(we::runtime::kindui::IconRenderer* iconRenderer) {
    m_Renderer = iconRenderer;
}

void ContentBrowserFolderArt::InvalidateCache() {
}

we::runtime::kindui::Rect ContentBrowserFolderArt::ComputeFolderRect(
    const we::runtime::kindui::Rect& bounds, float widthFill, float heightFill, bool alignBottom, float aspectRatio) {
    const float maxW = bounds.width * std::clamp(widthFill, 0.5f, 0.98f);
    const float maxH = bounds.height * std::clamp(heightFill, 0.5f, 0.98f);
    float width = maxW;
    float height = width / aspectRatio;
    if (height > maxH) {
        height = maxH;
        width = height * aspectRatio;
    }
    const float x = bounds.x + (bounds.width - width) * 0.5f;
    const float y = alignBottom
        ? bounds.y + bounds.height - height
        : bounds.y + (bounds.height - height) * 0.5f;
    return we::runtime::kindui::Rect{ x, y, width, height };
}

void ContentBrowserFolderArt::PaintFolderIcon(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& folderRect,
    bool hovered,
    bool opened) const
{
    // Sidebar / list: authored folder / folder-open at tree tier (24).
    const kindui::WindIconRef icon = opened ? WindIcons::FolderOpen24 : WindIcons::Folder24;
    if (!icon.IsValid()) {
        return;
    }
    context.DrawWindIcon(icon, folderRect, FolderTint(hovered));
}

void ContentBrowserFolderArt::PaintThumbnail(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& thumbRect,
    bool hovered) const
{
    // Grid thumbnails: authored content-folder_512 art (scaled into the slot).
    const we::runtime::kindui::Rect folderRect = ComputeFolderRect(thumbRect);
    if (!WindIcons::ContentFolder512.IsValid()) {
        PaintFolderIcon(context, folderRect, hovered, false);
        return;
    }
    context.DrawWindIcon(WindIcons::ContentFolder512, folderRect, FolderTint(hovered));
}

void ContentBrowserFolderArt::PaintSmallIcon(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& iconRect,
    bool hovered,
    bool opened) const
{
    const we::runtime::kindui::Rect folderRect = ComputeFolderRect(
        iconRect, kSmallIconWidthFill, kSmallIconHeightFill, false,
        opened ? kFolderOpenAspectRatio : kFolderAspectRatio);
    PaintFolderIcon(context, folderRect, hovered, opened);
}

} // namespace we::editor::contentbrowser
