#include "Services/ContentBrowserFolderArt.h"

#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"

#include <algorithm>
#include <cmath>

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
    // Atlas icons are resolved live — nothing to clear.
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
    if (folderRect.width < 1.0f || folderRect.height < 1.0f) {
        return;
    }

    // Atlas folders are near-white with prebaked lighting — tint must stay dark so
    // highlights don't blow out to neon orange-yellow.
    kindui::Color tint = kindui::ResolveColor(
        hovered ? kindui::ColorToken::ContentBrowserFolderHighlight
                : kindui::ColorToken::ContentBrowserFolderPrimary);
    const kindui::Color body = kindui::ResolveColor(kindui::ColorToken::ContentBrowserFolderBody);
    const kindui::Color edge = kindui::ResolveColor(kindui::ColorToken::ContentBrowserFolderEdge);
    if (hovered) {
        tint = kindui::Color{
            (tint.r * 0.70f) + (body.r * 0.30f),
            (tint.g * 0.70f) + (body.g * 0.30f),
            (tint.b * 0.70f) + (body.b * 0.30f),
            1.0f};
    } else {
        tint = kindui::Color{
            (body.r * 0.45f) + (edge.r * 0.55f),
            (body.g * 0.45f) + (edge.g * 0.55f),
            (body.b * 0.45f) + (edge.b * 0.55f),
            1.0f};
    }
    // Crush residual prebaked highlight energy from the atlas glyphs.
    constexpr float kPrebakedLightCrush = 0.72f;
    tint.r *= kPrebakedLightCrush;
    tint.g *= kPrebakedLightCrush;
    tint.b *= kPrebakedLightCrush;

    const char* iconName = opened ? kindui::Icons::OpenFolderName : kindui::Icons::FolderName;
    const float requestedPx = std::max(folderRect.width, folderRect.height);
    const uint32_t atlasTier = kindui::IconMetrics::SnapToAtlasTier(requestedPx);

    context.DrawIcon(iconName, folderRect, tint, static_cast<float>(atlasTier));
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
    const float aspectRatio = opened ? kFolderOpenAspectRatio : kFolderAspectRatio;
    we::runtime::kindui::Rect folderRect = ComputeFolderRect(
        iconRect, kSmallIconWidthFill, kSmallIconHeightFill, false, aspectRatio);
    folderRect.width = std::max(1.0f, std::round(folderRect.width));
    folderRect.height = std::max(1.0f, std::round(folderRect.height));
    folderRect.x = std::round(folderRect.x);
    folderRect.y = std::round(folderRect.y);
    PaintFolderIcon(context, folderRect, hovered, opened);
}

} // namespace we::editor::contentbrowser
