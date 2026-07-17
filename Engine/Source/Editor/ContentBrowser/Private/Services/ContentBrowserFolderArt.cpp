#include "Services/ContentBrowserFolderArt.h"
#include "Services/ThumbnailRenderer.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Rendering/IconRenderer.h"
#include <algorithm>
#include <cmath>

namespace we::editor::contentbrowser {

ContentBrowserFolderArt& ContentBrowserFolderArt::Get() {
    static ContentBrowserFolderArt instance;
    return instance;
}

void ContentBrowserFolderArt::Initialize(we::runtime::kindui::IconRenderer* iconRenderer) {
    m_Renderer = iconRenderer;
}

void ContentBrowserFolderArt::InvalidateCache() {
    m_Cache.clear();
}

we::runtime::kindui::Rect ContentBrowserFolderArt::ComputeFolderRect(
    const we::runtime::kindui::Rect& bounds, float widthFill, float heightFill, bool alignBottom, float aspectRatio) {
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
    return we::runtime::kindui::Rect{ x, y, width, height };
}

we::rhi::RHIDescriptorSetHandle ContentBrowserFolderArt::GetTexture(
    uint32_t widthPx, uint32_t heightPx, bool hovered, bool opened) const {
    if (!m_Renderer || heightPx == 0 || widthPx == 0) return we::rhi::RHIDescriptorSetHandle::Invalid;

    const float dpi = std::max(1.0f, we::runtime::kindui::DPIContext::GetScale());
    const uint32_t rasterHeight = std::max(16u, static_cast<uint32_t>(std::ceil(static_cast<float>(heightPx) * dpi)));

    const BitmapRGBA bitmap = ThumbnailRenderer::RenderContentBrowserFolder(
        rasterHeight, hovered ? 1.0f : 0.0f, opened);
    if (bitmap.pixels.empty()) return we::rhi::RHIDescriptorSetHandle::Invalid;

    const std::string key = "cb_folder_v17_" + std::to_string(bitmap.width) + "x" + std::to_string(bitmap.height)
        + (hovered ? "_h" : "_n") + (opened ? "_o" : "_c");

    auto it = m_Cache.find(key);
    if (it != m_Cache.end()) return it->second;

    const we::rhi::RHIDescriptorSetHandle texture = m_Renderer->CreateTextureFromBitmap(bitmap.pixels, bitmap.width, bitmap.height);
    if ((texture != we::rhi::RHIDescriptorSetHandle::Invalid)) {
        m_Cache[key] = texture;
    }
    return texture;
}

void ContentBrowserFolderArt::PaintThumbnail(we::runtime::kindui::PaintContext& context, const we::runtime::kindui::Rect& thumbRect, bool hovered) const {
    const we::runtime::kindui::Rect folderRect = ComputeFolderRect(thumbRect);
    const uint32_t heightPx = static_cast<uint32_t>(std::ceil(folderRect.height));
    const uint32_t widthPx = static_cast<uint32_t>(std::ceil(folderRect.width));
    const we::rhi::RHIDescriptorSetHandle texture = GetTexture(widthPx, heightPx, hovered, false);
    if ((texture != we::rhi::RHIDescriptorSetHandle::Invalid)) {
        context.DrawColorTexture(folderRect, texture);
    }
}

void ContentBrowserFolderArt::PaintSmallIcon(
    we::runtime::kindui::PaintContext& context, const we::runtime::kindui::Rect& iconRect, bool hovered, bool opened) const {
    const float aspectRatio = opened ? kFolderOpenAspectRatio : kFolderAspectRatio;
    we::runtime::kindui::Rect folderRect = ComputeFolderRect(
        iconRect, kSmallIconWidthFill, kSmallIconHeightFill, false, aspectRatio);
    folderRect.width = std::max(1.0f, std::round(folderRect.width));
    folderRect.height = std::max(1.0f, std::round(folderRect.height));
    folderRect.x = std::round(folderRect.x);
    folderRect.y = std::round(folderRect.y);

    const uint32_t widthPx = static_cast<uint32_t>(folderRect.width);
    const uint32_t heightPx = static_cast<uint32_t>(folderRect.height);
    const we::rhi::RHIDescriptorSetHandle texture = GetTexture(widthPx, heightPx, hovered, opened);
    if ((texture != we::rhi::RHIDescriptorSetHandle::Invalid)) {
        context.DrawColorTexture(folderRect, texture);
    }
}

} // namespace we::editor::contentbrowser
