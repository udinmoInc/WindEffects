#include "PlaceActors/PlaceActorsThumbnailProvider.h"

#include "PlaceActors/PlaceActorsIconProvider.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <algorithm>

namespace we::programs::editor {
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
namespace WEIcons = ::we::runtime::kindui::Icons;

PlaceActorsThumbnailProvider& PlaceActorsThumbnailProvider::Get() {
    static PlaceActorsThumbnailProvider instance;
    return instance;
}

PlaceActorsThumbnail PlaceActorsThumbnailProvider::Resolve(const PlaceActorsItemData& item) const {
    return Resolve(item.toolId);
}

PlaceActorsThumbnail PlaceActorsThumbnailProvider::Resolve(const std::string& toolId) const {
    PlaceActorsThumbnail thumb;

    const auto cacheIt = m_TextureCache.find(toolId);
    if (cacheIt != m_TextureCache.end() && cacheIt->second != nullptr) {
        thumb.kind = PlaceActorsThumbnailKind::CachedTexture;
        thumb.cachedTexture = cacheIt->second;
        return thumb;
    }

    const std::string previewIcon = PlaceActorsIconProvider::Get().ResolvePreviewIcon(toolId);
    if (!previewIcon.empty()) {
        thumb.kind = PlaceActorsThumbnailKind::AtlasIcon;
        thumb.atlasIconName = previewIcon;
        return thumb;
    }

    thumb.kind = PlaceActorsThumbnailKind::Placeholder;
    return thumb;
}

void PlaceActorsThumbnailProvider::Paint(PaintContext& context,
                                         const Rect& previewRect,
                                         const PlaceActorsItemData& item,
                                         float hoverAnim) const {
    if (previewRect.width <= 0.0f || previewRect.height <= 0.0f) {
        return;
    }

    const float radius = we::runtime::kindui::ResolveMetric(MetricToken::CornerRadiusSmall);
    const PlaceActorsThumbnail thumb = Resolve(item);

    // Shared preview frame — identical layout for placeholder, atlas icon, and future thumbnails.
    Color frame = we::runtime::kindui::ResolveColor(ColorToken::ActiveBackground);
    frame = Color::Lerp(frame, we::runtime::kindui::ResolveColor(ColorToken::PanelContentBackground), 0.35f);
    if (hoverAnim > 0.01f) {
        frame = Color::Lerp(frame, we::runtime::kindui::ResolveColor(ColorToken::HoverBackground), hoverAnim * 0.25f);
    }
    context.DrawRoundedRect(previewRect, frame, radius);
    context.DrawRoundedRectOutline(
        previewRect,
        we::runtime::kindui::ResolveColor(ColorToken::BorderDefault),
        1.0f,
        radius);

    if (thumb.kind == PlaceActorsThumbnailKind::CachedTexture && thumb.cachedTexture != nullptr) {
        // Future path: generated thumbnails drawn into the same preview rect.
        return;
    }

    if (thumb.kind == PlaceActorsThumbnailKind::AtlasIcon && !thumb.atlasIconName.empty()) {
        const float inset = std::max(4.0f, previewRect.width * 0.10f);
        const Rect iconRect{
            previewRect.x + inset,
            previewRect.y + inset,
            previewRect.width - inset * 2.0f,
            previewRect.height - inset * 2.0f
        };
        we::runtime::kindui::IconPainter::DrawIcon(
            context, thumb.atlasIconName, iconRect, Color::White());
        return;
    }

    // Neutral placeholder: centered muted icon when no thumbnail exists yet.
    const float iconSize = std::clamp(previewRect.width * 0.38f, 14.0f, 28.0f);
    const Rect iconRect{
        previewRect.x + (previewRect.width - iconSize) * 0.5f,
        previewRect.y + (previewRect.height - iconSize) * 0.5f,
        iconSize,
        iconSize
    };
    const std::string chromeIcon = PlaceActorsIconProvider::Get().ResolveChromeIcon(item);
    const std::string& placeholderIcon = !chromeIcon.empty() ? chromeIcon : WEIcons::PackageName;
    we::runtime::kindui::IconPainter::DrawIcon(
        context,
        placeholderIcon,
        iconRect,
        we::runtime::kindui::ResolveColor(ColorToken::IconSecondary));
}

void PlaceActorsThumbnailProvider::CacheTexture(const std::string& toolId, void* texture) {
    if (toolId.empty()) {
        return;
    }
    m_TextureCache[toolId] = texture;
}

void PlaceActorsThumbnailProvider::Invalidate(const std::string& toolId) {
    m_TextureCache.erase(toolId);
}

void PlaceActorsThumbnailProvider::InvalidateAll() {
    m_TextureCache.clear();
}

} // namespace we::programs::editor
