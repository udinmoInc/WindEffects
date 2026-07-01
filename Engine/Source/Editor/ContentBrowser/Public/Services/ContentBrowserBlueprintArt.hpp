#pragma once

#include "Core/Geometry.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <volk.h>

namespace we::UI {
class IconRenderer;
class PaintContext;
}

namespace we::editor::contentbrowser {

// Dedicated blueprint artwork for Content Browser (Assets/Editor/Visual_Graph.svg).
class ContentBrowserBlueprintArt {
public:
    static ContentBrowserBlueprintArt& Get();

    void Initialize(we::UI::IconRenderer* iconRenderer);
    void InvalidateCache();

    static constexpr float kThumbnailWidthFill = 0.84f;
    static constexpr float kThumbnailHeightFill = 0.80f;
    static constexpr float kBlueprintAspectRatio = 0.947f; // Assets/Editor/Visual_Graph.svg viewBox

    void PaintThumbnail(we::UI::PaintContext& context, const we::UI::Rect& thumbRect, bool hovered) const;
    void PaintSmallIcon(we::UI::PaintContext& context, const we::UI::Rect& iconRect, bool hovered) const;

    static we::UI::Rect ComputeBlueprintRect(const we::UI::Rect& bounds,
        float widthFill = kThumbnailWidthFill, float heightFill = kThumbnailHeightFill);

private:
    VkDescriptorSet GetTexture(uint32_t heightPx, bool hovered) const;

    we::UI::IconRenderer* m_Renderer = nullptr;
    mutable std::unordered_map<std::string, VkDescriptorSet> m_Cache;
};

} // namespace we::editor::contentbrowser
