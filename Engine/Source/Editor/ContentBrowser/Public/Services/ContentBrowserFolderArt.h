#pragma once

#include "Core/Geometry.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <volk.h>

namespace we::UI {
class IconRenderer;
class PaintContext;
}

namespace we::editor::contentbrowser {

// Dedicated UE5-style filled folder artwork for Content Browser (not Lucide).
class ContentBrowserFolderArt {
public:
    static ContentBrowserFolderArt& Get();

    void Initialize(we::UI::IconRenderer* iconRenderer);
    void InvalidateCache();

    static constexpr float kThumbnailWidthFill = 0.825f;
    static constexpr float kThumbnailHeightFill = 0.725f;
    static constexpr float kSmallIconWidthFill = 0.98f;
    static constexpr float kSmallIconHeightFill = 0.98f;
    static constexpr float kFolderAspectRatio = 231.0f / 203.0f; // Assets/Editor/Folder.svg
    static constexpr float kFolderOpenAspectRatio = 224.22424f / 182.99149f; // Assets/Editor/Folder_Open.svg

    void PaintThumbnail(we::UI::PaintContext& context, const we::UI::Rect& thumbRect, bool hovered) const;
    void PaintSmallIcon(we::UI::PaintContext& context, const we::UI::Rect& iconRect, bool hovered, bool opened = false) const;

    static we::UI::Rect ComputeFolderRect(const we::UI::Rect& bounds,
        float widthFill = kThumbnailWidthFill, float heightFill = kThumbnailHeightFill,
        bool alignBottom = true, float aspectRatio = kFolderAspectRatio);

private:
    VkDescriptorSet GetTexture(uint32_t widthPx, uint32_t heightPx, bool hovered, bool opened = false) const;

    we::UI::IconRenderer* m_Renderer = nullptr;
    mutable std::unordered_map<std::string, VkDescriptorSet> m_Cache;
};

} // namespace we::editor::contentbrowser
