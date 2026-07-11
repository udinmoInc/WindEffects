#pragma once

#include "Core/Geometry.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <volk.h>

namespace WindEffects::Editor::UI {
class IconRenderer;
class PaintContext;
}

namespace we::editor::contentbrowser {

// Dedicated blueprint artwork for Content Browser (Assets/Editor/Visual_Graph.svg).
class ContentBrowserBlueprintArt {
public:
    static ContentBrowserBlueprintArt& Get();

    void Initialize(WindEffects::Editor::UI::IconRenderer* iconRenderer);
    void InvalidateCache();

    static constexpr float kThumbnailWidthFill = 0.84f;
    static constexpr float kThumbnailHeightFill = 0.80f;
    static constexpr float kBlueprintAspectRatio = 0.947f; // Assets/Editor/Visual_Graph.svg viewBox

    void PaintThumbnail(WindEffects::Editor::UI::PaintContext& context, const WindEffects::Editor::UI::Rect& thumbRect, bool hovered) const;
    void PaintSmallIcon(WindEffects::Editor::UI::PaintContext& context, const WindEffects::Editor::UI::Rect& iconRect, bool hovered) const;

    static WindEffects::Editor::UI::Rect ComputeBlueprintRect(const WindEffects::Editor::UI::Rect& bounds,
        float widthFill = kThumbnailWidthFill, float heightFill = kThumbnailHeightFill);

private:
    VkDescriptorSet GetTexture(uint32_t heightPx, bool hovered) const;

    WindEffects::Editor::UI::IconRenderer* m_Renderer = nullptr;
    mutable std::unordered_map<std::string, VkDescriptorSet> m_Cache;
};

} // namespace we::editor::contentbrowser
