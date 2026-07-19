#pragma once

#include "KindUI/Core/Geometry.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Rendering/IconRenderer.h"
#include <cstdint>

namespace we::editor::contentbrowser {
using ::we::runtime::kindui::IconRenderer;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Rect;

/// Content Browser folder glyphs from the icon atlas (folder / openfolder tiers),
/// tinted with the dark orange–yellow Content Browser folder theme colors.
class ContentBrowserFolderArt {
public:
    static ContentBrowserFolderArt& Get();

    void Initialize(IconRenderer* iconRenderer);
    void InvalidateCache();

    static constexpr float kThumbnailWidthFill = 0.825f;
    static constexpr float kThumbnailHeightFill = 0.725f;
    static constexpr float kSmallIconWidthFill = 0.98f;
    static constexpr float kSmallIconHeightFill = 0.98f;
    // New atlas folder art aspect (e.g. 128x101, 16x13).
    static constexpr float kFolderAspectRatio = 128.0f / 101.0f;
    static constexpr float kFolderOpenAspectRatio = 128.0f / 101.0f;

    void PaintThumbnail(PaintContext& context, const Rect& thumbRect, bool hovered) const;
    void PaintSmallIcon(PaintContext& context, const Rect& iconRect, bool hovered, bool opened = false) const;

    static Rect ComputeFolderRect(const Rect& bounds,
        float widthFill = kThumbnailWidthFill, float heightFill = kThumbnailHeightFill,
        bool alignBottom = true, float aspectRatio = kFolderAspectRatio);

private:
    void PaintFolderIcon(PaintContext& context, const Rect& folderRect, bool hovered, bool opened) const;

    IconRenderer* m_Renderer = nullptr;
};

} // namespace we::editor::contentbrowser
