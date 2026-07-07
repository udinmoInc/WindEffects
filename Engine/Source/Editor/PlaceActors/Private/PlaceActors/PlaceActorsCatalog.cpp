#include "PlaceActors/PlaceActorsCatalog.h"

#include "EditorToolsRegistry.h"

#include <algorithm>
#include <unordered_map>

namespace we::programs::editor {

namespace {

struct ItemMeta {
    std::string description;
    std::vector<std::string> tags;
    std::vector<std::string> aliases;
};

const ItemMeta& MetaForTool(const std::string& toolId) {
    static const std::unordered_map<std::string, ItemMeta> kMeta = {
        {"PlaceEmptyActor", {"Empty transform root for grouping.", {"actor", "empty"}, {"empty", "null"}}},
        {"PlaceCharacter", {"Player or NPC character pawn.", {"character", "pawn"}, {"pawn", "player"}}},
        {"PlaceBlueprint", {"Logic-driven actor from a Blueprint class.", {"blueprint", "script"}, {"bp", "class"}}},
        {"PlaceCube", {"Static mesh cube primitive.", {"mesh", "primitive", "geometry"}, {"box", "block"}}},
        {"PlaceSphere", {"Static mesh sphere primitive.", {"mesh", "primitive", "geometry"}, {"ball", "orb"}}},
        {"PlacePlane", {"Flat ground or surface plane.", {"mesh", "primitive", "geometry"}, {"floor", "ground"}}},
        {"PlaceCylinder", {"Static mesh cylinder primitive.", {"mesh", "primitive", "geometry"}, {"tube", "column"}}},
        {"LightDirectional", {"Sun-like parallel light.", {"light", "sun"}, {"directional", "sun"}}},
        {"LightPoint", {"Omnidirectional point light.", {"light"}, {"point", "bulb"}}},
        {"LightSpot", {"Cone-shaped spotlight.", {"light"}, {"spot", "cone"}}},
        {"PlaceCamera", {"Scene camera actor.", {"camera", "cinematic"}, {"cam"}}},
        {"AudioPlace", {"3D audio source actor.", {"audio", "sound"}, {"sound", "speaker"}}},
        {"FXSpawn", {"Particle or visual effect emitter.", {"fx", "vfx"}, {"particle", "niagara"}}},
        {"UIWidget", {"Screen-space UI widget.", {"ui", "widget"}, {"hud", "canvas"}}},
        {"NavPaint", {"Navigation mesh modifier volume.", {"ai", "nav"}, {"navmesh"}}},
        {"PhysicsCollision", {"Physics collision volume.", {"physics", "collision"}, {"trigger", "volume"}}},
        {"FoliagePaintTool", {"Paint foliage instances on surfaces.", {"foliage", "paint"}, {"grass", "trees"}}},
        {"TerrainGenerate", {"Procedural terrain block.", {"landscape", "terrain"}, {"heightfield"}}},
        {"SplineDraw", {"Draw a spline path in the level.", {"spline", "path"}, {"curve", "road"}}},
        {"ModelingExtrude", {"Extrude selected mesh faces.", {"modeling", "mesh"}, {"edit", "geometry"}}},
        {"PaintVertex", {"Paint per-vertex colors.", {"paint", "mesh"}, {"vertex"}}},
        {"AnimRecord", {"Record animation keys.", {"animation"}, {"anim"}}},
        {"PhysicsSimulate", {"Run physics simulation.", {"physics"}, {"simulate"}}},
        {"NavBake", {"Bake navigation mesh.", {"navigation", "ai"}, {"navmesh"}}},
        {"AIBehaviorTree", {"AI behavior tree asset.", {"ai"}, {"behavior"}}},
        {"CineAddShot", {"Add cinematic camera shot.", {"cinematic", "camera"}, {"sequencer"}}},
    };

    static const ItemMeta kDefault{"Placeable editor actor.", {"actor"}, {}};
    auto it = kMeta.find(toolId);
    return it != kMeta.end() ? it->second : kDefault;
}

} // namespace

PlaceActorsCatalog& PlaceActorsCatalog::Get() {
    static PlaceActorsCatalog instance;
    return instance;
}

void PlaceActorsCatalog::Refresh() {
    BuildFromRegistry();
    m_Built = true;
}

void PlaceActorsCatalog::BuildFromRegistry() {
    m_Categories.clear();
    auto& registry = EditorToolsRegistry::Get();
    const auto* actorsMode = registry.FindMode("Actors");
    if (!actorsMode) {
        return;
    }

    for (const EditorToolCategory* category : registry.GetCategoriesForMode("Actors")) {
        PlaceActorsCategoryData categoryData;
        categoryData.id = category->id;
        categoryData.label = category->label;
        categoryData.iconName = category->iconName;
        categoryData.sortOrder = category->sortOrder;
        categoryData.defaultExpanded = category->defaultExpanded;

        for (const EditorToolAction* tool : registry.GetToolsForCategory(category->id)) {
            const ItemMeta& meta = MetaForTool(tool->id);
            PlaceActorsItemData item;
            item.toolId = tool->id;
            item.categoryId = category->id;
            item.categoryLabel = category->label;
            item.label = tool->label;
            item.iconName = tool->iconName;
            item.description = meta.description;
            item.tags = meta.tags;
            item.aliases = meta.aliases;
            item.sortOrder = tool->sortOrder;
            item.favoritable = tool->favoritable;
            categoryData.items.push_back(std::move(item));
        }

        if (!categoryData.items.empty()) {
            m_Categories.push_back(std::move(categoryData));
        }
    }

    std::sort(m_Categories.begin(), m_Categories.end(),
        [](const PlaceActorsCategoryData& a, const PlaceActorsCategoryData& b) {
            return a.sortOrder < b.sortOrder;
        });
}

std::vector<PlaceActorsItemData> PlaceActorsCatalog::GetAllItems() const {
    std::vector<PlaceActorsItemData> items;
    for (const auto& category : m_Categories) {
        items.insert(items.end(), category.items.begin(), category.items.end());
    }
    return items;
}

std::vector<std::string> PlaceActorsCatalog::GetCategoryFilterLabels() const {
    std::vector<std::string> labels = {"All"};
    for (const auto& category : m_Categories) {
        labels.push_back(category.label);
    }
    return labels;
}

const PlaceActorsItemData* PlaceActorsCatalog::FindItem(const std::string& toolId) const {
    for (const auto& category : m_Categories) {
        for (const auto& item : category.items) {
            if (item.toolId == toolId) {
                return &item;
            }
        }
    }
    return nullptr;
}

} // namespace we::programs::editor
