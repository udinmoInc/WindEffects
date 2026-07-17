#include "Services/ContentBrowserBlueprintArt.h"
#include "Services/ThumbnailRenderer.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Rendering/IconRenderer.h"
#include <algorithm>
#include <cmath>

namespace we::editor::contentbrowser {
using ::we::runtime::kindui::DPIContext;

ContentBrowserBlueprintArt& ContentBrowserBlueprintArt::Get() {
    static ContentBrowserBlueprintArt instance;
    return instance;
}

void ContentBrowserBlueprintArt::Initialize(we::runtime::kindui::IconRenderer* iconRenderer) {
    m_Renderer = iconRenderer;
}

void ContentBrowserBlueprintArt::InvalidateCache() {
    m_Cache.clear();
}

we::runtime::kindui::Rect ContentBrowserBlueprintArt::ComputeBlueprintRect(const we::runtime::kindui::Rect& bounds, float widthFill, float heightFill) {
    const float maxW = bounds.width * std::clamp(widthFill, 0.5f, 0.95f);
    const float maxH = bounds.height * std::clamp(heightFill, 0.5f, 0.95f);
    float width = maxW;
    float height = width / kBlueprintAspectRatio;
    if (height > maxH) {
        height = maxH;
        width = height * kBlueprintAspectRatio;
    }
    return we::runtime::kindui::Rect{
        bounds.x + (bounds.width - width) * 0.5f,
        bounds.y + (bounds.height - height) * 0.5f,
        width,
        height
    };
}

we::rhi::RHIDescriptorSetHandle ContentBrowserBlueprintArt::GetTexture(uint32_t heightPx, bool hovered) const {
    if (!m_Renderer || heightPx == 0) return we::rhi::RHIDescriptorSetHandle::Invalid;

    const float dpi = std::max(1.0f, we::runtime::kindui::DPIContext::GetScale());
    const uint32_t rasterHeight = std::max(16u, static_cast<uint32_t>(std::ceil(static_cast<float>(heightPx) * dpi)));
    const uint32_t rasterWidth = std::max(16u,
        static_cast<uint32_t>(std::round(static_cast<float>(rasterHeight) * kBlueprintAspectRatio)));

    const std::string key = "cb_blueprint_v2_" + std::to_string(rasterWidth) + "x" + std::to_string(rasterHeight)
        + (hovered ? "_h" : "_n");

    auto it = m_Cache.find(key);
    if (it != m_Cache.end()) return it->second;

    const BitmapRGBA bitmap = ThumbnailRenderer::RenderContentBrowserBlueprint(rasterHeight, hovered ? 1.0f : 0.0f);
    if (bitmap.pixels.empty()) return we::rhi::RHIDescriptorSetHandle::Invalid;

    const we::rhi::RHIDescriptorSetHandle texture = m_Renderer->CreateTextureFromBitmap(bitmap.pixels, bitmap.width, bitmap.height);
    if ((texture != we::rhi::RHIDescriptorSetHandle::Invalid)) {
        m_Cache[key] = texture;
    }
    return texture;
}

void ContentBrowserBlueprintArt::PaintThumbnail(we::runtime::kindui::PaintContext& context, const we::runtime::kindui::Rect& thumbRect, bool hovered) const {
    const we::runtime::kindui::Rect blueprintRect = ComputeBlueprintRect(thumbRect);
    const uint32_t heightPx = static_cast<uint32_t>(std::ceil(blueprintRect.height));
    const we::rhi::RHIDescriptorSetHandle texture = GetTexture(heightPx, hovered);
    if ((texture != we::rhi::RHIDescriptorSetHandle::Invalid)) {
        context.DrawTexture(blueprintRect, texture);
    }
}

void ContentBrowserBlueprintArt::PaintSmallIcon(we::runtime::kindui::PaintContext& context, const we::runtime::kindui::Rect& iconRect, bool hovered) const {
    const we::runtime::kindui::Rect blueprintRect = ComputeBlueprintRect(iconRect, 0.88f, 0.88f);
    const uint32_t heightPx = static_cast<uint32_t>(std::ceil(blueprintRect.height));
    const we::rhi::RHIDescriptorSetHandle texture = GetTexture(heightPx, hovered);
    if ((texture != we::rhi::RHIDescriptorSetHandle::Invalid)) {
        context.DrawTexture(blueprintRect, texture);
    }
}

} // namespace we::editor::contentbrowser
