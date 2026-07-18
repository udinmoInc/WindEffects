#pragma once

#include "KindUI/Core/Geometry.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Rendering/IconRenderer.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include "RHI/Types.h"

namespace we::editor::contentbrowser {
using ::we::runtime::kindui::IconRenderer;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Rect;

// Dedicated blueprint artwork for Content Browser (Assets/Editor/Visual_Graph.svg).
class ContentBrowserBlueprintArt {
public:
    static ContentBrowserBlueprintArt& Get();

    void Initialize(IconRenderer* iconRenderer);
    void InvalidateCache();

    static constexpr float kThumbnailWidthFill = 0.84f;
    static constexpr float kThumbnailHeightFill = 0.80f;
    static constexpr float kBlueprintAspectRatio = 0.947f; // Assets/Editor/Visual_Graph.svg viewBox

    void PaintThumbnail(PaintContext& context, const Rect& thumbRect, bool hovered) const;
    void PaintSmallIcon(PaintContext& context, const Rect& iconRect, bool hovered) const;

    static Rect ComputeBlueprintRect(const Rect& bounds,
        float widthFill = kThumbnailWidthFill, float heightFill = kThumbnailHeightFill);

private:
    we::rhi::RHIDescriptorSetHandle GetTexture(uint32_t heightPx, bool hovered) const;

    IconRenderer* m_Renderer = nullptr;
    mutable std::unordered_map<std::string, we::rhi::RHIDescriptorSetHandle> m_Cache;
};

} // namespace we::editor::contentbrowser
