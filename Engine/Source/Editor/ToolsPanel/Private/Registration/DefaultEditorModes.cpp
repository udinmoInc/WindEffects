// ==============================================================================
// WindEffects — ToolsPanel — DefaultEditorModes
// Internal implementation for the ToolsPanel module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"

namespace we::programs::editor {
using ::we::runtime::kindui::kWindIconNone;
namespace WindIcons = ::we::runtime::kindui::WindIcons;

namespace {

void RegisterBuiltinEditorModes() {
    // Modes are registered via static initializers below.
}

struct BuiltinModeBootstrap {
    BuiltinModeBootstrap() { RegisterBuiltinEditorModes(); }
};
static BuiltinModeBootstrap g_BuiltinModeBootstrap;

} // namespace

// ===== Editor Modes (extensible via REGISTER_EDITOR_TOOL_MODE in plugins) =====
REGISTER_EDITOR_TOOL_MODE_COMPACT_WITH_TOOLTIP(Select,      "Select",      WindIcons::ToolbarHand16, 10,
    "Select, move, rotate, and scale scene objects (Q, W, E, R)")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(Actors,      "Actors",      WindIcons::Cube2516,      20,
    "Place geometry, lights, cameras, shapes, and actor classes")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(Landscape,   "Landscape",   WindIcons::Grid16,       30,
    "Sculpt terrain heightmaps and paint landscape materials")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(Foliage,     "Foliage",     WindIcons::Cloud16,     40,
    "Paint trees, plants, grass, and instanced static meshes")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(Terrain,     "Terrain",     WindIcons::Earth16,      50,
    "Generate procedural landscapes or import heightmaps")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(Spline,      "Spline",      WindIcons::RedoAlt16,       60,
    "Draw gameplay splines, paths, and control point curves")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(Modeling,    "Modeling",    WindIcons::BoxSolid16,       70,
    "Extrude, inset, bevel, and perform 3D mesh modeling")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(Paint,       "Paint",       WindIcons::Brush16,      80,
    "Paint vertex colors, textures, and material blend weights")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(Animation,   "Animation",   WindIcons::Clapperboard16,   90,
    "Record keyframes, scrub timeline, and edit skeletal clips")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(Physics,     "Physics",     WindIcons::Box16,       100,
    "Simulate rigid bodies, collision meshes, and force fields")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(Navigation,  "Navigation",  WindIcons::FolderSearch16,    110,
    "Bake AI navmesh geometry and edit pathfinding bounds")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(FX,          "FX",          WindIcons::Bulb16,       120,
    "Spawn particle systems, Niagara emitters, and visual effects")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(AI,          "AI",          WindIcons::CircleHelp16,       130,
    "Author behavior trees, blackboards, and AI agent logic")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(Audio,       "Audio",       WindIcons::Speaker16,       140,
    "Place spatialized sound actors and ambient audio probes")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(UI,          "UI",          WindIcons::Window16,       150,
    "Author in-game UI widgets, HUD layouts, and interactive canvas")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(Lighting,    "Lighting",    WindIcons::Sun16,      160,
    "Place directional, point, spot lights and bake scene lighting")
REGISTER_EDITOR_TOOL_MODE_WITH_TOOLTIP(Cinematics,  "Cinematics",  WindIcons::ToolbarVideocamera16,     170,
    "Add cinematic cameras and edit camera tracks in Sequencer")

// ===== Select mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Select, SelectEssentials, "Essentials", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(SelectEssentials, SelectTool,   "Select",   WindIcons::BoxSolid16, "Q", [](){})
REGISTER_EDITOR_TOOL(SelectEssentials, MoveTool,     "Move",     WindIcons::AdjustHorizon16,   "W", [](){})
REGISTER_EDITOR_TOOL(SelectEssentials, RotateTool,   "Rotate",   WindIcons::RedoAlt16, "E", [](){})
REGISTER_EDITOR_TOOL(SelectEssentials, ScaleTool,    "Scale",    WindIcons::ToolbarScaling16,  "R", [](){})

// ===== Actors mode catalog is registered by WindEffects-PlaceActors =====

// ===== Landscape mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Landscape, LandscapeSculpt, "Sculpt", WindIcons::Grid16, 10)
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptRaise,   "Raise",   WindIcons::Box16,  "", [](){})
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptLower,   "Lower",   WindIcons::Minus16, "", [](){})
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptSmooth,  "Smooth",  WindIcons::Refresh16, "", [](){})
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptFlatten, "Flatten", WindIcons::Square16, "", [](){})

// ===== Foliage mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Foliage, FoliagePaint, "Paint", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(FoliagePaint, FoliagePaintTool, "Paint Foliage", kWindIconNone, "Shift+4", [](){})
REGISTER_EDITOR_TOOL(FoliagePaint, FoliageErase,    "Erase Foliage", kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(FoliagePaint, FoliageSelect,   "Select Instance", kWindIconNone, "", [](){})

// ===== Terrain mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Terrain, TerrainTools, "Terrain", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(TerrainTools, TerrainGenerate, "Generate Terrain", WindIcons::Grid16, "", [](){})
REGISTER_EDITOR_TOOL(TerrainTools, TerrainImport,   "Import Heightmap", WindIcons::FolderSearch16, "", [](){})

// ===== Spline mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Spline, SplineTools, "Splines", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(SplineTools, SplineDraw,   "Draw Spline",   kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(SplineTools, SplineEdit,   "Edit Control Points", kWindIconNone, "", [](){})

// ===== Modeling mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Modeling, ModelingOps, "Mesh Operations", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(ModelingOps, ModelingExtrude, "Extrude", kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(ModelingOps, ModelingInset,   "Inset",   WindIcons::ToolbarScaling16, "", [](){})
REGISTER_EDITOR_TOOL(ModelingOps, ModelingBevel,   "Bevel",   kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(ModelingOps, ModelingBoolean, "Boolean", kWindIconNone, "", [](){})

// ===== Paint mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Paint, PaintTools, "Painting", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(PaintTools, PaintVertex,  "Vertex Paint",  kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(PaintTools, PaintTexture, "Texture Paint", kWindIconNone, "", [](){})

// ===== Animation mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Animation, AnimationTools, "Animation", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(AnimationTools, AnimRecord, "Record", WindIcons::PlayForward16, "", [](){})
REGISTER_EDITOR_TOOL(AnimationTools, AnimScrub,  "Scrub Timeline", kWindIconNone, "", [](){})

// ===== Physics mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Physics, PhysicsTools, "Physics", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(PhysicsTools, PhysicsSimulate, "Simulate", kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(PhysicsTools, PhysicsCollision, "Edit Collision", kWindIconNone, "", [](){})

// ===== Navigation mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Navigation, NavigationTools, "Navigation", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(NavigationTools, NavBake,  "Build NavMesh", WindIcons::Grid16, "", [](){})
REGISTER_EDITOR_TOOL(NavigationTools, NavPaint, "Paint Nav Area", kWindIconNone, "", [](){})

// ===== FX mode =====
REGISTER_EDITOR_TOOL_CATEGORY(FX, FXTools, "Effects", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(FXTools, FXSpawn, "Spawn Emitter", kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(FXTools, FXBake,  "Bake Niagara", WindIcons::Refresh16, "", [](){})

// ===== AI mode =====
REGISTER_EDITOR_TOOL_CATEGORY(AI, AITools, "AI", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(AITools, AIBehaviorTree, "Behavior Tree", kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(AITools, AIBlackboard,   "Blackboard",    kWindIconNone, "", [](){})

// ===== Audio mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Audio, AudioTools, "Audio", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(AudioTools, AudioPlace, "Place Sound", kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(AudioTools, AudioProbe, "Audio Probe", WindIcons::Search16, "", [](){})

// ===== UI mode =====
REGISTER_EDITOR_TOOL_CATEGORY(UI, UITools, "UI Authoring", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(UITools, UIWidget, "Widget", kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(UITools, UILayout, "Layout Grid", WindIcons::Grid16, "", [](){})

// ===== Lighting mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Lighting, LightingTools, "Lighting", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(LightingTools, LightDirectional, "Directional Light", WindIcons::Sun16, "", [](){})
REGISTER_EDITOR_TOOL(LightingTools, LightPoint,       "Point Light",       WindIcons::Bulb16, "", [](){})
REGISTER_EDITOR_TOOL(LightingTools, LightBuild,       "Build Lighting",    WindIcons::Bulb16, "", [](){})

// ===== Cinematics mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Cinematics, CinematicsTools, "Sequencer", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(CinematicsTools, CineAddShot,   "Add Camera", WindIcons::ToolbarCamera16, "", [](){})
REGISTER_EDITOR_TOOL(CinematicsTools, CineKeyframe,  "Keyframe",   WindIcons::ToolbarVideocamera16, "", [](){})

} // namespace we::programs::editor
