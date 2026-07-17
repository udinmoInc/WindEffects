#pragma once

#include "PlaceActors/Export.h"
#include "PlaceActors/PlaceActorsTypes.h"
#include "PlaceActors/PlaceActorsItem.h"
#include "KindUI/Core/Geometry.h"
#include "KindUI/Core/PaintContext.h"

namespace we::programs::editor {

// Compact asset-browser card: large preview, centered icon/thumbnail, name beneath.
class PLACEACTORS_API PlaceActorsActorCard {
public:
    static void Paint(we::runtime::kindui::PaintContext& context,
                      const we::runtime::kindui::Rect& cardBounds,
                      const we::runtime::kindui::Rect& previewBounds,
                      const PlaceActorsItemData& item,
                      float hoverAnim,
                      float pressAnim,
                      bool selected,
                      bool favorite);

    static bool HitFavoriteStar(const we::runtime::kindui::Rect& cardBounds,
                                const we::runtime::kindui::Point& position);
};

} // namespace we::programs::editor
