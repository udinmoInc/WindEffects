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

// Dedicated UE5-style filled folder artwork for Content Browser (not Lucide).
class ContentBrowserFolderArt {
public:
    static ContentBrowserFolderArt& Get();

    void Initialize(IconRenderer* iconRenderer);
    void InvalidateCache();

    static constexpr float kThumbnailWidthFill = 0.825f;
    static constexpr float kThumbnailHeightFill = 0.725f;
    static constexpr float kSmallIconWidthFill = 0.98f;
    static constexpr float kSmallIconHeightFill = 0.98f;
    static constexpr float kFolderAspectRatio = 231.0f / 203.0f; // Assets/Editor/Folder.svg
    static constexpr float kFolderOpenAspectRatio = 224.22424f / 182.99149f; // Assets/Editor/Folder_Open.svg

    void PaintThumbnail(PaintContext& context, const Rect& thumbRect, bool hovered) const;
    void PaintSmallIcon(PaintContext& context, const Rect& iconRect, bool hovered, bool opened = false) const;

    static Rect ComputeFolderRect(const Rect& bounds,
        float widthFill = kThumbnailWidthFill, float heightFill = kThumbnailHeightFill,
        bool alignBottom = true, float aspectRatio = kFolderAspectRatio);

private:
    we::rhi::RHIDescriptorSetHandle GetTexture(uint32_t widthPx, uint32_t heightPx, bool hovered, bool opened = false) const;

    IconRenderer* m_Renderer = nullptr;
    mutable std::unordered_map<std::string, we::rhi::RHIDescriptorSetHandle> m_Cache;
};

} // namespace we::editor::contentbrowser
