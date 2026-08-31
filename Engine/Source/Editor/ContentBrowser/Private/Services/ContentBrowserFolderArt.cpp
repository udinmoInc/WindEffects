#include "Services/ContentBrowserFolderArt.h"

#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/PaintContext.h"

namespace we::editor::contentbrowser {
namespace kindui = ::we::runtime::kindui;

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
    (void)context;
    (void)folderRect;
    (void)hovered;
    (void)opened;
}

void ContentBrowserFolderArt::PaintThumbnail(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& thumbRect,
    bool hovered) const
{
    const we::runtime::kindui::Rect folderRect = ComputeFolderRect(thumbRect);
    PaintFolderIcon(context, folderRect, hovered, false);
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
