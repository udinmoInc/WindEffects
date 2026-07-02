#include "Services/ContentBrowserFolderArt.hpp"
#include "Services/ThumbnailRenderer.hpp"
#include "Core/DPIContext.hpp"
#include "Core/PaintContext.hpp"
#include "Rendering/IconRenderer.hpp"
#include <algorithm>
#include <cmath>

namespace we::editor::contentbrowser {

ContentBrowserFolderArt& ContentBrowserFolderArt::Get() {
    static ContentBrowserFolderArt instance;
    return instance;
}

void ContentBrowserFolderArt::Initialize(we::UI::IconRenderer* iconRenderer) {
    m_Renderer = iconRenderer;
}

void ContentBrowserFolderArt::InvalidateCache() {
    m_Cache.clear();
}

we::UI::Rect ContentBrowserFolderArt::ComputeFolderRect(const we::UI::Rect& bounds, float widthFill, float heightFill, bool alignBottom) {
    const float maxW = bounds.width * std::clamp(widthFill, 0.5f, 0.95f);
    const float maxH = bounds.height * std::clamp(heightFill, 0.5f, 0.95f);
    float width = maxW;
    float height = width / kFolderAspectRatio;
    if (height > maxH) {
        height = maxH;
        width = height * kFolderAspectRatio;
    }
    const float x = bounds.x + (bounds.width - width) * 0.5f;
    const float y = alignBottom
        ? bounds.y + bounds.height - height
        : bounds.y + (bounds.height - height) * 0.5f;
    return we::UI::Rect{ x, y, width, height };
}

VkDescriptorSet ContentBrowserFolderArt::GetTexture(uint32_t heightPx, bool hovered) const {
    if (!m_Renderer || heightPx == 0) return VK_NULL_HANDLE;

    const float dpi = std::max(1.0f, we::UI::DPIContext::GetScale());
    const uint32_t rasterHeight = std::max(16u, static_cast<uint32_t>(std::ceil(static_cast<float>(heightPx) * dpi)));
    const uint32_t rasterWidth = std::max(16u,
        static_cast<uint32_t>(std::round(static_cast<float>(rasterHeight) * kFolderAspectRatio)));

    const std::string key = "cb_folder_v9_" + std::to_string(rasterWidth) + "x" + std::to_string(rasterHeight)
        + (hovered ? "_h" : "_n");

    auto it = m_Cache.find(key);
    if (it != m_Cache.end()) return it->second;

    const BitmapRGBA bitmap = ThumbnailRenderer::RenderContentBrowserFolder(rasterHeight, hovered ? 1.0f : 0.0f);
    if (bitmap.pixels.empty()) return VK_NULL_HANDLE;

    const VkDescriptorSet texture = m_Renderer->CreateTextureFromBitmap(bitmap.pixels, bitmap.width, bitmap.height);
    if (texture != VK_NULL_HANDLE) {
        m_Cache[key] = texture;
    }
    return texture;
}

void ContentBrowserFolderArt::PaintThumbnail(we::UI::PaintContext& context, const we::UI::Rect& thumbRect, bool hovered) const {
    const we::UI::Rect folderRect = ComputeFolderRect(thumbRect);
    const uint32_t heightPx = static_cast<uint32_t>(std::ceil(folderRect.height));
    const VkDescriptorSet texture = GetTexture(heightPx, hovered);
    if (texture != VK_NULL_HANDLE) {
        context.DrawTexture(folderRect, texture);
    }
}

void ContentBrowserFolderArt::PaintSmallIcon(we::UI::PaintContext& context, const we::UI::Rect& iconRect, bool hovered) const {
    const we::UI::Rect folderRect = ComputeFolderRect(iconRect, kSmallIconWidthFill, kSmallIconHeightFill, false);
    const uint32_t heightPx = static_cast<uint32_t>(std::ceil(folderRect.height));
    const VkDescriptorSet texture = GetTexture(heightPx, hovered);
    if (texture != VK_NULL_HANDLE) {
        context.DrawTexture(folderRect, texture);
    }
}

} // namespace we::editor::contentbrowser
