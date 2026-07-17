#pragma once

#include "KindUI/Core/Geometry.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include "RHI/Types.h"

namespace we::runtime::kindui {
class IconRenderer;
class PaintContext;
}

namespace we::editor::contentbrowser {

// Dedicated blueprint artwork for Content Browser (Assets/Editor/Visual_Graph.svg).
class ContentBrowserBlueprintArt {
public:
    static ContentBrowserBlueprintArt& Get();

    void Initialize(we::runtime::kindui::IconRenderer* iconRenderer);
    void InvalidateCache();

    static constexpr float kThumbnailWidthFill = 0.84f;
    static constexpr float kThumbnailHeightFill = 0.80f;
    static constexpr float kBlueprintAspectRatio = 0.947f; // Assets/Editor/Visual_Graph.svg viewBox

    void PaintThumbnail(we::runtime::kindui::PaintContext& context, const we::runtime::kindui::Rect& thumbRect, bool hovered) const;
    void PaintSmallIcon(we::runtime::kindui::PaintContext& context, const we::runtime::kindui::Rect& iconRect, bool hovered) const;

    static we::runtime::kindui::Rect ComputeBlueprintRect(const we::runtime::kindui::Rect& bounds,
        float widthFill = kThumbnailWidthFill, float heightFill = kThumbnailHeightFill);

private:
    we::rhi::RHIDescriptorSetHandle GetTexture(uint32_t heightPx, bool hovered) const;

    we::runtime::kindui::IconRenderer* m_Renderer = nullptr;
    mutable std::unordered_map<std::string, we::rhi::RHIDescriptorSetHandle> m_Cache;
};

} // namespace we::editor::contentbrowser
