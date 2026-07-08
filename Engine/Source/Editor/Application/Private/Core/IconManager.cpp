#include "Core/IconManager.h"
#include "Core/DPIContext.h"
#include "Core/PaintContext.h"
#include "Core/Theme.h"
#include "Rendering/IconRenderer.h"
#include <algorithm>
#include <cmath>

namespace we::UI {

IconManager& IconManager::Get() {
    static IconManager instance;
    return instance;
}

void IconManager::Initialize(IconRenderer* renderer) {
    m_Renderer = renderer;
}

void IconManager::InvalidateCache() {
    if (m_Renderer) {
        m_Renderer->ClearCache();
    }
}

Color IconManager::GetFolderIconColor(bool hovered) const {
    Color color = Theme::Get().IconFolder;
    if (hovered) {
        color = Color::Lerp(color, Theme::Get().IconHover, 0.18f);
    }
    return color;
}

VkDescriptorSet IconManager::GetFolderIconTexture(uint32_t sizePx, const Color& tint) const {
    if (!m_Renderer || sizePx == 0) return VK_NULL_HANDLE;
    const float dpi = std::max(1.0f, DPIContext::GetScale());
    const uint32_t rasterSize = static_cast<uint32_t>(std::ceil(static_cast<float>(sizePx) * dpi));
    return m_Renderer->GetLucideIcon(FolderIcon, std::max(8u, rasterSize), tint, 0.0f);
}

Rect IconManager::ComputeFolderIconRect(const Rect& thumbRect, float fillRatio) {
    const float clamped = std::clamp(fillRatio, 0.5f, 0.9f);
    const float size = std::min(thumbRect.width, thumbRect.height) * clamped;
    return Rect{
        thumbRect.x + (thumbRect.width - size) * 0.5f,
        thumbRect.y + (thumbRect.height - size) * 0.5f,
        size,
        size
    };
}

void IconManager::PaintFolderListIcon(PaintContext& context, const Rect& iconRect, bool hovered) const {
    const Color iconColor = GetFolderIconColor(hovered);
    IconPainter::DrawIcon(context, FolderIcon, iconRect, iconColor);
}

} // namespace we::UI
