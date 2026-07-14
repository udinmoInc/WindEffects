#include "PlaceActors/PlaceActorsPanel.h"
#include "PlaceActors/PlaceActorsPlacement.h"
#include "EditorToolsRegistry.h"
#include "Core/Icon.h"

#include <memory>
#include <string>
#include <vector>

namespace we::programs::editor {

namespace {

namespace WEIcons = WindEffects::Editor::UI::Icons;

void RegisterCategory(const char* modeId, const char* categoryId, const char* label, const char* icon,
    int sortOrder, bool defaultExpanded = false)
{
    EditorToolCategory category;
    category.id = categoryId;
    category.modeId = modeId;
    category.label = label;
    category.iconName = icon;
    category.sortOrder = sortOrder;
    category.defaultExpanded = defaultExpanded;
    EditorToolsRegistry::Get().RegisterCategory(std::move(category));
}

void RegisterTool(const char* categoryId,
                  const char* toolId,
                  const char* label,
                  const char* icon,
                  int sortOrder,
                  const std::vector<std::string>& keywords = {})
{
    EditorToolAction tool;
    tool.id = toolId;
    tool.categoryId = categoryId;
    tool.label = label;
    tool.iconName = icon;
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
    RegisterCategory("Actors", "ActorBasic", "Basic", WEIcons::CrosshairName, 10, true);
    RegisterCategory("Actors", "ActorGeometry", "Geometry", WEIcons::CubeName, 20, true);
    RegisterCategory("Actors", "ActorLights", "Lights", WEIcons::LightName, 30);
    RegisterCategory("Actors", "ActorCameras", "Cameras", WEIcons::CameraName, 40);
    RegisterCategory("Actors", "ActorCharacters", "Characters", WEIcons::UserName, 50);
    RegisterCategory("Actors", "ActorEnvironment", "Environment", WEIcons::MountainName, 60);
    RegisterCategory("Actors", "ActorCinematics", "Cinematics", WEIcons::VideoName, 70);
    RegisterCategory("Actors", "ActorAudio", "Audio", WEIcons::Volume2Name, 80);
    RegisterCategory("Actors", "ActorFX", "Visual Effects", WEIcons::SparklesName, 90);
    RegisterCategory("Actors", "ActorVolumes", "Volumes", WEIcons::LayersName, 100);
    RegisterCategory("Actors", "ActorAllClasses", "All Classes", WEIcons::ListName, 110);

    RegisterTool("ActorBasic", "PlaceEmptyActor", "Empty Actor", WEIcons::CrosshairName, 10, {"empty", "actor", "transform"});
    RegisterTool("ActorBasic", "PlaceEmptyCharacter", "Empty Character", WEIcons::UserName, 20, {"character", "pawn", "empty"});
    RegisterTool("ActorBasic", "PlaceEmptyPawn", "Empty Pawn", WEIcons::UserName, 30, {"pawn", "empty"});
    RegisterTool("ActorBasic", "LightPoint", "Point Light", WEIcons::PointLightName, 40, {"point", "omni", "light"});
    RegisterTool("ActorBasic", "LightSpot", "Spot Light", WEIcons::FlashlightName, 50, {"spot", "cone", "light"});

    RegisterTool("ActorGeometry", "PlaceCube", "Cube", WEIcons::CubeName, 10, {"box", "mesh", "geometry"});
    RegisterTool("ActorGeometry", "PlaceSphere", "Sphere", WEIcons::SphereName, 20, {"ball", "mesh", "geometry"});
    RegisterTool("ActorGeometry", "PlaceCylinder", "Cylinder", WEIcons::CylinderName, 30, {"tube", "mesh", "geometry"});
    RegisterTool("ActorGeometry", "PlacePlane", "Plane", WEIcons::PlaneName, 40, {"floor", "mesh", "geometry"});
    RegisterTool("ActorGeometry", "PlaceCone", "Cone", WEIcons::ConeName, 50, {"cone", "mesh", "geometry"});
    RegisterTool("ActorGeometry", "PlaceCapsule", "Capsule", WEIcons::CapsuleName, 60, {"capsule", "mesh", "geometry"});

    RegisterTool("ActorLights", "LightDirectional", "Directional Light", WEIcons::SunName, 10, {"sun", "directional", "light"});
    RegisterTool("ActorLights", "LightPoint", "Point Light", WEIcons::PointLightName, 20, {"point", "omni", "light"});
    RegisterTool("ActorLights", "LightSpot", "Spot Light", WEIcons::FlashlightName, 30, {"spot", "cone", "light"});

    RegisterTool("ActorCameras", "PlaceCamera", "Camera", WEIcons::CameraName, 10, {"camera", "cine", "view"});

    RegisterTool("ActorCharacters", "PlaceCharacter", "Character", WEIcons::UserName, 10, {"pawn", "character", "player"});

    RegisterTool("ActorEnvironment", "TerrainGenerate", "Landscape", WEIcons::MountainName, 10, {"terrain", "heightfield", "environment"});
    RegisterTool("ActorEnvironment", "FoliagePaintTool", "Foliage", WEIcons::TreesName, 20, {"foliage", "grass", "environment"});

    RegisterTool("ActorCinematics", "CineAddShot", "Cinematic Camera", WEIcons::VideoName, 10, {"sequencer", "cinematic", "shot"});

    RegisterTool("ActorAudio", "AudioPlace", "Audio Source", WEIcons::Volume2Name, 10, {"sound", "audio", "speaker"});

    RegisterTool("ActorFX", "FXSpawn", "Particle System", WEIcons::SparklesName, 10, {"vfx", "niagara", "particle"});

    RegisterTool("ActorVolumes", "NavPaint", "Nav Modifier Volume", WEIcons::MapName, 10, {"volume", "nav", "navigation"});
    RegisterTool("ActorVolumes", "PhysicsCollision", "Physics Volume", WEIcons::CubeName, 20, {"collision", "trigger", "volume"});

    RegisterTool("ActorAllClasses", "PlaceBlueprint", "Blueprint", WEIcons::BlocksName, 10, {"blueprint", "class", "script"});
    RegisterTool("ActorAllClasses", "PlaceBlueprintClass", "Blueprint Class", WEIcons::BlocksName, 20, {"blueprint", "class"});
    RegisterTool("ActorAllClasses", "ModelingExtrude", "Geometry Brush", WEIcons::CubeName, 30, {"brush", "geometry", "modeling"});
    RegisterTool("ActorAllClasses", "UIWidget", "UI Widget", WEIcons::LayoutPanelName, 40, {"widget", "hud", "ui"});
    RegisterTool("ActorAllClasses", "AIBehaviorTree", "AI Controller", WEIcons::BrainName, 50, {"behavior", "ai"});
    RegisterTool("ActorAllClasses", "NavBake", "Nav Mesh Bounds", WEIcons::CompassName, 60, {"navigation", "navmesh"});
    RegisterTool("ActorAllClasses", "SplineDraw", "Gameplay Trigger", WEIcons::ZapName, 70, {"trigger", "gameplay", "spline"});
    RegisterTool("ActorAllClasses", "PlaceNote", "Editor Note", WEIcons::StickyNoteName, 80, {"note", "comment", "utility"});
}

void ConfigureActorsModePanel() {
    EditorToolMode mode;
    if (const auto* existing = EditorToolsRegistry::Get().FindMode("Actors")) {
        mode = *existing;
    } else {
        mode.id = "Actors";
        mode.label = "Assets";
        mode.iconName = WEIcons::AddActorName;
        mode.sortOrder = 20;
        mode.keywords = "Actors Place Assets";
        mode.opensToolDrawerByDefault = true;
    }

    mode.customContent = [](const EditorToolMode&, const std::string& searchFilter) {
        auto panel = std::make_shared<PlaceActorsPanel>();
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

} // namespace

} // namespace we::programs::editor
