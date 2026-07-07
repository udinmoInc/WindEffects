#pragma once

#include "Application/Export.h"

#include "Core/Geometry.h"
#include "Core/Icon.h"
#include <cstdint>
#include <volk.h>

namespace we::UI {

class PaintContext;
class IconRenderer;

// Shared semantic icon access â€” toolbar/tree vs content-browser thumbnail variants.
class APPLICATION_API IconManager {
public:
    static IconManager& Get();

    void Initialize(IconRenderer* renderer);
    void InvalidateCache();

    static constexpr const char* FolderIcon = Icons::FolderName;
    static constexpr float FolderThumbnailFillRatio = 0.78f;

    Color GetFolderIconColor(bool hovered = false) const;

    VkDescriptorSet GetFolderIconTexture(uint32_t sizePx, const Color& tint) const;

    static Rect ComputeFolderIconRect(const Rect& thumbRect, float fillRatio = 0.625f);

    void PaintFolderListIcon(PaintContext& context, const Rect& iconRect, bool hovered) const;

private:
    IconRenderer* m_Renderer = nullptr;
};

} // namespace we::UI
