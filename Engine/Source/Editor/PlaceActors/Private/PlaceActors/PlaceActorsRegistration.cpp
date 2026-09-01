#include "PlaceActors/PlaceActorsRegistration.h"
#include "PlaceActors/PlaceActorsPanel.h"
#include "PlaceActors/PlaceActorsPlacement.h"
#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"

#include <memory>
#include <string>
#include <vector>

namespace we::programs::editor {
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;
using ::we::editor::toolspanel::EditorToolsRegistry;
using ::we::editor::toolspanel::EditorToolCategory;
using ::we::editor::toolspanel::EditorToolAction;
using ::we::editor::toolspanel::EditorToolMode;


namespace {

void RegisterCategory(const char* modeId, const char* categoryId, const char* label, we::runtime::kindui::WindIconRef icon,
    int sortOrder, bool defaultExpanded = false)
{
    EditorToolCategory category;
    category.id = categoryId;
    category.modeId = modeId;
    category.label = label;
    category.icon = icon;
    category.sortOrder = sortOrder;
    category.defaultExpanded = defaultExpanded;
    EditorToolsRegistry::Get().RegisterCategory(std::move(category));
}

void RegisterTool(const char* categoryId,
                  const char* toolId,
                  const char* label,
                  we::runtime::kindui::WindIconRef icon,
                  int sortOrder,
                  const std::vector<std::string>& keywords = {})
{
    EditorToolAction tool;
    tool.id = toolId;
    tool.categoryId = categoryId;
    tool.label = label;
    tool.icon = icon;
    tool.sortOrder = sortOrder;
    if (!keywords.empty()) {
        std::string joined;
        for (size_t i = 0; i < keywords.size(); ++i) {
            if (i > 0) joined += ' ';
            joined += keywords[i];
        }
        tool.keywords = joined;
    } else {
        tool.keywords = label;
    }
    tool.onExecute = []() {};
    tool.onDragStart = [toolId = std::string(toolId)]() {
        PlaceActorsPlacement::Get().BeginDragPlacement(toolId);
    };
    EditorToolsRegistry::Get().RegisterTool(std::move(tool));
}

void RegisterActorCatalog() {
    // Basic category removed — Quick Access is synthesized by PlaceActorsPanel.
    RegisterCategory("Actors", "ActorGeometry", "Geometry", WindIcons::BoxSolid16, 20, false);
    RegisterCategory("Actors", "ActorLights", "Lights", WindIcons::Bulb16, 30, false);
    RegisterCategory("Actors", "ActorCameras", "Cameras", WindIcons::Camera16, 40, false);
    RegisterCategory("Actors", "ActorCharacters", "Characters", WindIcons::Mouse16, 50, false);
    RegisterCategory("Actors", "ActorEnvironment", "Environment", WindIcons::WorldGlobe16, 60, false);
    RegisterCategory("Actors", "ActorCinematics", "Cinematics", WindIcons::VideoCamera16, 70, false);
    RegisterCategory("Actors", "ActorAudio", "Audio", WindIcons::AlertNormal16, 80, false);
    RegisterCategory("Actors", "ActorFX", "Visual Effects", WindIcons::Cloud16, 90, false);
    RegisterCategory("Actors", "ActorVolumes", "Volumes", WindIcons::Box16, 100, false);
    RegisterCategory("Actors", "ActorAllClasses", "All Classes", WindIcons::FolderClosed16, 110, false);

    RegisterTool("ActorGeometry", "PlaceCube", "Cube", WindIcons::Box16, 10, {"box", "mesh", "geometry"});
    RegisterTool("ActorGeometry", "PlaceSphere", "Sphere", kWindIconNone, 20, {"ball", "mesh", "geometry"});
    RegisterTool("ActorGeometry", "PlaceCylinder", "Cylinder", kWindIconNone, 30, {"tube", "mesh", "geometry"});
    RegisterTool("ActorGeometry", "PlacePlane", "Plane", WindIcons::Square16, 40, {"floor", "mesh", "geometry"});
    RegisterTool("ActorGeometry", "PlaceCone", "Cone", kWindIconNone, 50, {"cone", "mesh", "geometry"});
    RegisterTool("ActorGeometry", "PlaceCapsule", "Capsule", kWindIconNone, 60, {"capsule", "mesh", "geometry"});

    // Empty transforms live under Characters / All Classes for catalog search; Quick Access surfaces them.
    RegisterTool("ActorCharacters", "PlaceEmptyActor", "Empty Actor", kWindIconNone, 5, {"empty", "actor", "transform"});
    RegisterTool("ActorCharacters", "PlaceEmptyCharacter", "Empty Character", kWindIconNone, 8, {"character", "pawn", "empty"});
    RegisterTool("ActorCharacters", "PlaceEmptyPawn", "Empty Pawn", kWindIconNone, 9, {"pawn", "empty"});
    RegisterTool("ActorCharacters", "PlaceCharacter", "Character", kWindIconNone, 10, {"pawn", "character", "player"});

    RegisterTool("ActorLights", "LightDirectional", "Directional Light", we::runtime::kindui::WindIcons::Sun16, 10, {"sun", "directional", "light"});
    RegisterTool("ActorLights", "LightPoint", "Point Light", WindIcons::Bulb16, 20, {"point", "omni", "light"});
    RegisterTool("ActorLights", "LightSpot", "Spot Light", WindIcons::Bulb16, 30, {"spot", "cone", "light"});

    RegisterTool("ActorCameras", "PlaceCamera", "Camera", WindIcons::Camera16, 10, {"camera", "cine", "view"});

    RegisterTool("ActorEnvironment", "TerrainGenerate", "Landscape", WindIcons::Grid3x316, 10, {"terrain", "heightfield", "environment"});
    RegisterTool("ActorEnvironment", "FoliagePaintTool", "Foliage", kWindIconNone, 20, {"foliage", "grass", "environment"});

    RegisterTool("ActorCinematics", "CineAddShot", "Cinematic Camera", WindIcons::VideoCamera16, 10, {"sequencer", "cinematic", "shot"});

    RegisterTool("ActorAudio", "AudioPlace", "Audio Source", kWindIconNone, 10, {"sound", "audio", "speaker"});

    RegisterTool("ActorFX", "FXSpawn", "Particle System", kWindIconNone, 10, {"vfx", "niagara", "particle"});

    RegisterTool("ActorVolumes", "NavPaint", "Nav Modifier Volume", kWindIconNone, 10, {"volume", "nav", "navigation"});
    RegisterTool("ActorVolumes", "PhysicsCollision", "Physics Volume", kWindIconNone, 20, {"collision", "trigger", "volume"});

    RegisterTool("ActorAllClasses", "PlaceBlueprint", "Blueprint", kWindIconNone, 10, {"blueprint", "class", "script"});
    RegisterTool("ActorAllClasses", "PlaceBlueprintClass", "Blueprint Class", kWindIconNone, 20, {"blueprint", "class"});
    RegisterTool("ActorAllClasses", "ModelingExtrude", "Geometry Brush", kWindIconNone, 30, {"brush", "geometry", "modeling"});
    RegisterTool("ActorAllClasses", "UIWidget", "UI Widget", WindIcons::Mouse16, 40, {"widget", "hud", "ui"});
    RegisterTool("ActorAllClasses", "AIBehaviorTree", "AI Controller", kWindIconNone, 50, {"behavior", "ai"});
    RegisterTool("ActorAllClasses", "NavBake", "Nav Mesh Bounds", kWindIconNone, 60, {"navigation", "navmesh"});
    RegisterTool("ActorAllClasses", "SplineDraw", "Gameplay Trigger", kWindIconNone, 70, {"trigger", "gameplay", "spline"});
    RegisterTool("ActorAllClasses", "PlaceNote", "Editor Note", WindIcons::InfoNormal16, 80, {"note", "comment", "utility"});
}

void ConfigureActorsModePanel() {
    EditorToolMode mode;
    if (const auto* existing = EditorToolsRegistry::Get().FindMode("Actors")) {
        mode = *existing;
    } else {
        mode.id = "Actors";
        mode.label = "Assets";
        mode.icon = kWindIconNone;
        mode.sortOrder = 20;
        mode.keywords = "Actors Place Assets";
        mode.opensToolDrawerByDefault = true;
    }

    mode.customContent = [](const EditorToolMode&, const std::string& searchFilter) {
        auto panel = std::make_shared<PlaceActorsPanel>();
        panel->InitializeCallbacks(panel);
        panel->SetExternalSearchFilter(searchFilter);
        return panel;
    };
    EditorToolsRegistry::Get().RegisterMode(std::move(mode));
}

struct PlaceActorsRegistration {
    PlaceActorsRegistration() {
        RegisterActorCatalog();
        ConfigureActorsModePanel();
    }
};

static PlaceActorsRegistration g_PlaceActorsRegistration;

void EnsureRegisteredImpl() {
    RegisterActorCatalog();
    ConfigureActorsModePanel();
}

} // namespace

void EnsurePlaceActorsRegistered() {
    // Re-apply in case an earlier static registrar overwrote Actors mode without customContent.
    EnsureRegisteredImpl();
}

} // namespace we::programs::editor
