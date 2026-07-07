#include "PlaceActors/PlaceActorsPanel.h"
#include "PlaceActors/PlaceActorsPlacement.h"
#include "EditorToolsRegistry.h"

#include <memory>
#include <string>
#include <vector>

namespace we::programs::editor {

namespace {

void RegisterCategory(const char* modeId, const char* categoryId, const char* label, const char* icon, int sortOrder) {
    EditorToolCategory category;
    category.id = categoryId;
    category.modeId = modeId;
    category.label = label;
    category.iconName = icon;
    category.sortOrder = sortOrder;
    EditorToolsRegistry::Get().RegisterCategory(std::move(category));
}

void RegisterTool(const char* categoryId,
                  const char* toolId,
                  const char* label,
                  const char* icon,
                  int sortOrder,
                  const std::vector<std::string>& keywords = {}) {
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
    RegisterCategory("Actors", "ActorPlacement", "Placement", "plus", 10);
    RegisterCategory("Actors", "ActorBlueprints", "Blueprints", "code", 20);
    RegisterCategory("Actors", "ActorLights", "Lights", "light", 30);
    RegisterCategory("Actors", "ActorCameras", "Cameras", "camera", 40);
    RegisterCategory("Actors", "ActorMeshes", "Meshes", "cube", 50);
    RegisterCategory("Actors", "ActorGeometry", "Geometry", "grid", 60);
    RegisterCategory("Actors", "ActorCharacters", "Characters", "hierarchy", 70);
    RegisterCategory("Actors", "ActorPhysics", "Physics", "snap", 80);
    RegisterCategory("Actors", "ActorAudio", "Audio", "info", 90);
    RegisterCategory("Actors", "ActorFX", "Visual Effects", "star", 100);
    RegisterCategory("Actors", "ActorLandscape", "Landscape", "grid", 110);
    RegisterCategory("Actors", "ActorVolumes", "Volumes", "package", 120);
    RegisterCategory("Actors", "ActorUI", "UI", "menu", 130);
    RegisterCategory("Actors", "ActorAI", "AI", "code", 140);
    RegisterCategory("Actors", "ActorNavigation", "Navigation", "compass", 150);
    RegisterCategory("Actors", "ActorGameplay", "Gameplay", "play", 160);
    RegisterCategory("Actors", "ActorUtilities", "Utilities", "settings", 170);

    RegisterTool("ActorPlacement", "PlaceEmptyActor", "Empty Actor", "plus", 10, {"empty", "actor"});
    RegisterTool("ActorPlacement", "PlaceBlueprint", "Blueprint", "code", 20, {"blueprint", "class"});
    RegisterTool("ActorBlueprints", "PlaceBlueprintClass", "Blueprint Class", "code", 10, {"blueprint"});
    RegisterTool("ActorLights", "LightDirectional", "Directional Light", "sun", 10, {"sun", "directional"});
    RegisterTool("ActorLights", "LightPoint", "Point Light", "point-light", 20, {"point", "omni"});
    RegisterTool("ActorLights", "LightSpot", "Spot Light", "light", 30, {"spot", "cone"});
    RegisterTool("ActorCameras", "PlaceCamera", "Camera", "camera", 10, {"camera", "cine"});
    RegisterTool("ActorMeshes", "PlaceCube", "Cube", "cube", 10, {"box", "mesh"});
    RegisterTool("ActorMeshes", "PlaceSphere", "Sphere", "sphere", 20, {"ball", "mesh"});
    RegisterTool("ActorMeshes", "PlaceCylinder", "Cylinder", "cylinder", 30, {"tube", "mesh"});
    RegisterTool("ActorMeshes", "PlacePlane", "Plane", "plane", 40, {"floor", "mesh"});
    RegisterTool("ActorGeometry", "ModelingExtrude", "Geometry Brush", "cube", 10, {"brush", "geometry"});
    RegisterTool("ActorCharacters", "PlaceCharacter", "Character", "hierarchy", 10, {"pawn", "character"});
    RegisterTool("ActorPhysics", "PhysicsCollision", "Physics Volume", "cube", 10, {"collision", "trigger"});
    RegisterTool("ActorAudio", "AudioPlace", "Audio Source", "info", 10, {"sound", "audio"});
    RegisterTool("ActorFX", "FXSpawn", "Particle System", "star", 10, {"vfx", "niagara"});
    RegisterTool("ActorLandscape", "TerrainGenerate", "Landscape", "grid", 10, {"terrain", "heightfield"});
    RegisterTool("ActorVolumes", "NavPaint", "Nav Modifier Volume", "compass", 10, {"volume", "nav"});
    RegisterTool("ActorUI", "UIWidget", "UI Widget", "menu", 10, {"widget", "hud"});
    RegisterTool("ActorAI", "AIBehaviorTree", "AI Controller", "hierarchy", 10, {"behavior", "ai"});
    RegisterTool("ActorNavigation", "NavBake", "Nav Mesh Bounds", "compass", 10, {"navigation"});
    RegisterTool("ActorGameplay", "SplineDraw", "Gameplay Trigger", "play", 10, {"trigger", "gameplay"});
    RegisterTool("ActorUtilities", "PlaceNote", "Editor Note", "document", 10, {"note", "comment"});
}

void ConfigureActorsModePanel() {
    EditorToolMode mode;
    if (const auto* existing = EditorToolsRegistry::Get().FindMode("Actors")) {
        mode = *existing;
    } else {
        mode.id = "Actors";
        mode.label = "Actors";
        mode.iconName = "hierarchy";
        mode.sortOrder = 20;
        mode.keywords = "Actors Place";
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
