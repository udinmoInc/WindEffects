#include "Services/ContentBrowserFolderArt.h"
#include "Services/ThumbnailRenderer.h"
#include "Core/DPIContext.h"
#include "Core/PaintContext.h"
#include "Rendering/IconRenderer.h"
#include <algorithm>
#include <cmath>

namespace we::editor::contentbrowser {

ContentBrowserFolderArt& ContentBrowserFolderArt::Get() {
    static ContentBrowserFolderArt instance;
    return instance;
}

void ContentBrowserFolderArt::Initialize(WindEffects::Editor::UI::IconRenderer* iconRenderer) {
    m_Renderer = iconRenderer;
}

void ContentBrowserFolderArt::InvalidateCache() {
    m_Cache.clear();
}

WindEffects::Editor::UI::Rect ContentBrowserFolderArt::ComputeFolderRect(
    const WindEffects::Editor::UI::Rect& bounds, float widthFill, float heightFill, bool alignBottom, float aspectRatio) {
    const float maxW = bounds.width * std::clamp(widthFill, 0.5f, 0.95f);
    const float maxH = bounds.height * std::clamp(heightFill, 0.5f, 0.95f);
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
    return WindEffects::Editor::UI::Rect{ x, y, width, height };
}

VkDescriptorSet ContentBrowserFolderArt::GetTexture(
    uint32_t widthPx, uint32_t heightPx, bool hovered, bool opened) const {
    if (!m_Renderer || heightPx == 0 || widthPx == 0) return VK_NULL_HANDLE;

    const float dpi = std::max(1.0f, WindEffects::Editor::UI::DPIContext::GetScale());
    const uint32_t rasterHeight = std::max(16u, static_cast<uint32_t>(std::ceil(static_cast<float>(heightPx) * dpi)));

    const BitmapRGBA bitmap = ThumbnailRenderer::RenderContentBrowserFolder(
        rasterHeight, hovered ? 1.0f : 0.0f, opened);
    if (bitmap.pixels.empty()) return VK_NULL_HANDLE;

    const std::string key = "cb_folder_v17_" + std::to_string(bitmap.width) + "x" + std::to_string(bitmap.height)
        + (hovered ? "_h" : "_n") + (opened ? "_o" : "_c");

    auto it = m_Cache.find(key);
    if (it != m_Cache.end()) return it->second;

    const VkDescriptorSet texture = m_Renderer->CreateTextureFromBitmap(bitmap.pixels, bitmap.width, bitmap.height);
    if (texture != VK_NULL_HANDLE) {
        m_Cache[key] = texture;
    }
    return texture;
}

void ContentBrowserFolderArt::PaintThumbnail(WindEffects::Editor::UI::PaintContext& context, const WindEffects::Editor::UI::Rect& thumbRect, bool hovered) const {
    const WindEffects::Editor::UI::Rect folderRect = ComputeFolderRect(thumbRect);
    const uint32_t heightPx = static_cast<uint32_t>(std::ceil(folderRect.height));
    const uint32_t widthPx = static_cast<uint32_t>(std::ceil(folderRect.width));
    const VkDescriptorSet texture = GetTexture(widthPx, heightPx, hovered, false);
    if (texture != VK_NULL_HANDLE) {
        context.DrawColorTexture(folderRect, texture);
    }
}

void ContentBrowserFolderArt::PaintSmallIcon(
    WindEffects::Editor::UI::PaintContext& context, const WindEffects::Editor::UI::Rect& iconRect, bool hovered, bool opened) const {
    const float aspectRatio = opened ? kFolderOpenAspectRatio : kFolderAspectRatio;
    WindEffects::Editor::UI::Rect folderRect = ComputeFolderRect(
        iconRect, kSmallIconWidthFill, kSmallIconHeightFill, false, aspectRatio);
    folderRect.width = std::max(1.0f, std::round(folderRect.width));
    folderRect.height = std::max(1.0f, std::round(folderRect.height));
    folderRect.x = std::round(folderRect.x);
    folderRect.y = std::round(folderRect.y);

    const uint32_t widthPx = static_cast<uint32_t>(folderRect.width);
    const uint32_t heightPx = static_cast<uint32_t>(folderRect.height);
    const VkDescriptorSet texture = GetTexture(widthPx, heightPx, hovered, opened);
    if (texture != VK_NULL_HANDLE) {
        context.DrawColorTexture(folderRect, texture);
    }
}

} // namespace we::editor::contentbrowser
