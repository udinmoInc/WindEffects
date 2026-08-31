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
REGISTER_EDITOR_TOOL_MODE_COMPACT(Select,      "Select",      WindIcons::Box16,     10)
REGISTER_EDITOR_TOOL_MODE(Actors,      "Actors",      kWindIconNone,  20)
REGISTER_EDITOR_TOOL_MODE(Landscape,   "Landscape",   WindIcons::Grid3x316,       30)
REGISTER_EDITOR_TOOL_MODE(Foliage,     "Foliage",     kWindIconNone,     40)
REGISTER_EDITOR_TOOL_MODE(Terrain,     "Terrain",     kWindIconNone,      50)
REGISTER_EDITOR_TOOL_MODE(Spline,      "Spline",      kWindIconNone,       60)
REGISTER_EDITOR_TOOL_MODE(Modeling,    "Modeling",    kWindIconNone,       70)
REGISTER_EDITOR_TOOL_MODE(Paint,       "Paint",       kWindIconNone,   80)
REGISTER_EDITOR_TOOL_MODE(Animation,   "Animation",   kWindIconNone,       90)
REGISTER_EDITOR_TOOL_MODE(Physics,     "Physics",     kWindIconNone,       100)
REGISTER_EDITOR_TOOL_MODE(Navigation,  "Navigation",  kWindIconNone,    110)
REGISTER_EDITOR_TOOL_MODE(FX,          "FX",          kWindIconNone,       120)
REGISTER_EDITOR_TOOL_MODE(AI,          "AI",          kWindIconNone,       130)
REGISTER_EDITOR_TOOL_MODE(Audio,       "Audio",       kWindIconNone,       140)
REGISTER_EDITOR_TOOL_MODE(UI,          "UI",          kWindIconNone,       150)
REGISTER_EDITOR_TOOL_MODE(Lighting,    "Lighting",    WindIcons::Sun16,      160)
REGISTER_EDITOR_TOOL_MODE(Cinematics,  "Cinematics",  kWindIconNone,     170)

// ===== Select mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Select, SelectEssentials, "Essentials", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(SelectEssentials, SelectTool,   "Select",   WindIcons::Box16, "Q", [](){})
REGISTER_EDITOR_TOOL(SelectEssentials, MoveTool,     "Move",     kWindIconNone,   "W", [](){})
REGISTER_EDITOR_TOOL(SelectEssentials, RotateTool,   "Rotate",   kWindIconNone, "E", [](){})
REGISTER_EDITOR_TOOL(SelectEssentials, ScaleTool,    "Scale",    WindIcons::Scaling16,  "R", [](){})

// ===== Actors mode catalog is registered by WindEffects-PlaceActors =====

// ===== Landscape mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Landscape, LandscapeSculpt, "Sculpt", WindIcons::Grid3x316, 10)
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptRaise,   "Raise",   WindIcons::Box16,  "", [](){})
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptLower,   "Lower",   WindIcons::Minus16, "", [](){})
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptSmooth,  "Smooth",  WindIcons::Refresh16, "", [](){})
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptFlatten, "Flatten", kWindIconNone, "", [](){})

// ===== Foliage mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Foliage, FoliagePaint, "Paint", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(FoliagePaint, FoliagePaintTool, "Paint Foliage", kWindIconNone, "Shift+4", [](){})
REGISTER_EDITOR_TOOL(FoliagePaint, FoliageErase,    "Erase Foliage", kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(FoliagePaint, FoliageSelect,   "Select Instance", kWindIconNone, "", [](){})

// ===== Terrain mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Terrain, TerrainTools, "Terrain", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(TerrainTools, TerrainGenerate, "Generate Terrain", WindIcons::Grid3x316, "", [](){})
REGISTER_EDITOR_TOOL(TerrainTools, TerrainImport,   "Import Heightmap", kWindIconNone, "", [](){})

// ===== Spline mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Spline, SplineTools, "Splines", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(SplineTools, SplineDraw,   "Draw Spline",   kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(SplineTools, SplineEdit,   "Edit Control Points", kWindIconNone, "", [](){})

// ===== Modeling mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Modeling, ModelingOps, "Mesh Operations", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(ModelingOps, ModelingExtrude, "Extrude", kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(ModelingOps, ModelingInset,   "Inset",   WindIcons::Scaling16, "", [](){})
REGISTER_EDITOR_TOOL(ModelingOps, ModelingBevel,   "Bevel",   kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(ModelingOps, ModelingBoolean, "Boolean", kWindIconNone, "", [](){})

// ===== Paint mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Paint, PaintTools, "Painting", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(PaintTools, PaintVertex,  "Vertex Paint",  kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(PaintTools, PaintTexture, "Texture Paint", kWindIconNone, "", [](){})

// ===== Animation mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Animation, AnimationTools, "Animation", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(AnimationTools, AnimRecord, "Record", kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(AnimationTools, AnimScrub,  "Scrub Timeline", kWindIconNone, "", [](){})

// ===== Physics mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Physics, PhysicsTools, "Physics", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(PhysicsTools, PhysicsSimulate, "Simulate", kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(PhysicsTools, PhysicsCollision, "Edit Collision", kWindIconNone, "", [](){})

// ===== Navigation mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Navigation, NavigationTools, "Navigation", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(NavigationTools, NavBake,  "Build NavMesh", WindIcons::Grid3x316, "", [](){})
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
REGISTER_EDITOR_TOOL(UITools, UILayout, "Layout Grid", WindIcons::Grid3x316, "", [](){})

// ===== Lighting mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Lighting, LightingTools, "Lighting", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(LightingTools, LightDirectional, "Directional Light", WindIcons::Sun16, "", [](){})
REGISTER_EDITOR_TOOL(LightingTools, LightPoint,       "Point Light",       kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(LightingTools, LightBuild,       "Build Lighting",    kWindIconNone, "", [](){})

// ===== Cinematics mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Cinematics, CinematicsTools, "Sequencer", kWindIconNone, 10)
REGISTER_EDITOR_TOOL(CinematicsTools, CineAddShot,   "Add Camera", kWindIconNone, "", [](){})
REGISTER_EDITOR_TOOL(CinematicsTools, CineKeyframe,  "Keyframe",   kWindIconNone, "", [](){})

} // namespace we::programs::editor
