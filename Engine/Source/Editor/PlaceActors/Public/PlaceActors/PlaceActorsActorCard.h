#pragma once

#include "PlaceActors/Export.h"
#include "PlaceActors/PlaceActorsTypes.h"
#include "PlaceActors/PlaceActorsItem.h"
#include "Core/Geometry.h"
#include "Core/PaintContext.h"

namespace we::programs::editor {

// Compact asset-browser card: large preview, centered icon/thumbnail, name beneath.
class PLACEACTORS_API PlaceActorsActorCard {
public:
    static void Paint(WindEffects::Editor::UI::PaintContext& context,
                      const WindEffects::Editor::UI::Rect& cardBounds,
                      const WindEffects::Editor::UI::Rect& previewBounds,
                      const PlaceActorsItemData& item,
                      float hoverAnim,
                      float pressAnim,
                      bool selected,
                      bool favorite);

    static bool HitFavoriteStar(const WindEffects::Editor::UI::Rect& cardBounds,
                                const WindEffects::Editor::UI::Point& position);
};

} // namespace we::programs::editor
