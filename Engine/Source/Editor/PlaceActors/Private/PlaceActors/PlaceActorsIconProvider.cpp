#include "PlaceActors/PlaceActorsIconProvider.h"

#include "Core/Icon.h"

namespace we::programs::editor {

namespace WEIcons = WindEffects::Editor::UI::Icons;

PlaceActorsIconProvider& PlaceActorsIconProvider::Get() {
    static PlaceActorsIconProvider instance;
    return instance;
}

std::string PlaceActorsIconProvider::ResolveChromeIcon(const PlaceActorsItemData& item) const {
    // Prefer flat chrome glyphs for list/row UI; 3D atlas marks stay in previews.
    if (item.toolId == "PlaceCube") {
        return WEIcons::CubeName;
    }
    if (item.toolId == "PlaceSphere") {
        return WEIcons::SphereName;
    }
    if (item.toolId == "PlaceCylinder") {
        return WEIcons::CylinderName;
    }
    if (item.toolId == "PlacePlane") {
        return WEIcons::PlaneName;
    }
    if (item.toolId == "PlaceCone") {
        return WEIcons::ConeName;
    }
    if (item.toolId == "PlaceCapsule") {
        return WEIcons::CapsuleName;
    }
    if (item.toolId == "PlaceEmptyActor") {
        return WEIcons::CrosshairName;
    }
    return item.iconName;
}

std::string PlaceActorsIconProvider::ResolvePreviewIcon(const PlaceActorsItemData& item) const {
    return ResolvePreviewIcon(item.toolId);
}

std::string PlaceActorsIconProvider::ResolvePreviewIcon(const std::string& toolId) const {
    // Full-color atlas thumbnails (Icons_3D* / Icons_3d*) for grid cards.
    if (toolId == "PlaceCube") {
        return WEIcons::Cube3DName;
    }
    if (toolId == "PlaceSphere") {
        return WEIcons::Sphere3DName;
    }
    if (toolId == "PlaceCylinder") {
        return WEIcons::Cylinder3DName;
    }
    if (toolId == "PlacePlane") {
        return WEIcons::Plane3DName;
    }
    if (toolId == "PlaceCone") {
        return WEIcons::Cone3DName;
    }
    if (toolId == "PlaceCapsule") {
        return WEIcons::Capsule3DName;
    }
    if (toolId == "PlaceEmptyActor") {
        return WEIcons::BlankActor3DName;
    }
    if (toolId == "PlaceCamera") {
        return WEIcons::CameraName;
    }
    return {};
}

bool PlaceActorsIconProvider::HasPreviewIcon(const std::string& toolId) const {
    return !ResolvePreviewIcon(toolId).empty();
}

} // namespace we::programs::editor
