#pragma once

#include "PlaceActors/Export.h"
#include "PlaceActors/PlaceActorsTypes.h"
#include "Core/Geometry.h"
#include "Core/PaintContext.h"
#include <string>
#include <unordered_map>

namespace we::programs::editor {

enum class PlaceActorsThumbnailKind {
    AtlasIcon,
    Placeholder,
    CachedTexture
};

struct PlaceActorsThumbnail {
    PlaceActorsThumbnailKind kind = PlaceActorsThumbnailKind::Placeholder;
    std::string atlasIconName;
    void* cachedTexture = nullptr; // reserved for future generated thumbnails
};

// Abstracts preview source so cards switch between icons and thumbnails without layout changes.
class PLACEACTORS_API PlaceActorsThumbnailProvider {
public:
    static PlaceActorsThumbnailProvider& Get();

    [[nodiscard]] PlaceActorsThumbnail Resolve(const PlaceActorsItemData& item) const;
    [[nodiscard]] PlaceActorsThumbnail Resolve(const std::string& toolId) const;

    void Paint(WindEffects::Editor::UI::PaintContext& context,
               const WindEffects::Editor::UI::Rect& previewRect,
               const PlaceActorsItemData& item,
               float hoverAnim = 0.0f) const;

    // Reserved for future async thumbnail generation / cache invalidation.
    void CacheTexture(const std::string& toolId, void* texture);
    void Invalidate(const std::string& toolId);
    void InvalidateAll();

private:
    PlaceActorsThumbnailProvider() = default;
    mutable std::unordered_map<std::string, void*> m_TextureCache;
};

} // namespace we::programs::editor
