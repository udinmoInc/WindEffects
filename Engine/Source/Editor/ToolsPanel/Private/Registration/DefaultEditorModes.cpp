#include "EditorToolsRegistry.h"

namespace we::programs::editor {

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
REGISTER_EDITOR_TOOL_MODE_COMPACT(Select,      "Select",      "cursor",     10)
REGISTER_EDITOR_TOOL_MODE(Actors,      "Actors",      "hierarchy",  20)
REGISTER_EDITOR_TOOL_MODE(Landscape,   "Landscape",   "grid",       30)
REGISTER_EDITOR_TOOL_MODE(Foliage,     "Foliage",     "layers",     40)
REGISTER_EDITOR_TOOL_MODE(Terrain,     "Terrain",     "plane",      50)
REGISTER_EDITOR_TOOL_MODE(Spline,      "Spline",      "move",       60)
REGISTER_EDITOR_TOOL_MODE(Modeling,    "Modeling",    "cube",       70)
REGISTER_EDITOR_TOOL_MODE(Paint,       "Paint",       "material",   80)
REGISTER_EDITOR_TOOL_MODE(Animation,   "Animation",   "play",       90)
REGISTER_EDITOR_TOOL_MODE(Physics,     "Physics",     "snap",       100)
REGISTER_EDITOR_TOOL_MODE(Navigation,  "Navigation",  "compass",    110)
REGISTER_EDITOR_TOOL_MODE(FX,          "FX",          "star",       120)
REGISTER_EDITOR_TOOL_MODE(AI,          "AI",          "code",       130)
REGISTER_EDITOR_TOOL_MODE(Audio,       "Audio",       "info",       140)
REGISTER_EDITOR_TOOL_MODE(UI,          "UI",          "menu",       150)
REGISTER_EDITOR_TOOL_MODE(Lighting,    "Lighting",    "light",      160)
REGISTER_EDITOR_TOOL_MODE(Cinematics,  "Cinematics",  "camera",     170)

// ===== Select mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Select, SelectEssentials, "Essentials", "cursor", 10)
REGISTER_EDITOR_TOOL(SelectEssentials, SelectTool,   "Select",   "cursor", "Q", [](){})
REGISTER_EDITOR_TOOL(SelectEssentials, MoveTool,     "Move",     "move",   "W", [](){})
REGISTER_EDITOR_TOOL(SelectEssentials, RotateTool,   "Rotate",   "rotate", "E", [](){})
REGISTER_EDITOR_TOOL(SelectEssentials, ScaleTool,    "Scale",    "scale",  "R", [](){})

// ===== Actors mode catalog is registered by WindEffects-PlaceActors =====

// ===== Landscape mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Landscape, LandscapeSculpt, "Sculpt", "grid", 10)
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptRaise,   "Raise",   "plus",  "", [](){})
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptLower,   "Lower",   "minus", "", [](){})
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptSmooth,  "Smooth",  "refresh", "", [](){})
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptFlatten, "Flatten", "plane", "", [](){})

// ===== Foliage mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Foliage, FoliagePaint, "Paint", "layers", 10)
REGISTER_EDITOR_TOOL(FoliagePaint, FoliagePaintTool, "Paint Foliage", "layers", "Shift+4", [](){})
REGISTER_EDITOR_TOOL(FoliagePaint, FoliageErase,    "Erase Foliage", "eraser", "", [](){})
REGISTER_EDITOR_TOOL(FoliagePaint, FoliageSelect,   "Select Instance", "cursor", "", [](){})

// ===== Terrain mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Terrain, TerrainTools, "Terrain", "plane", 10)
REGISTER_EDITOR_TOOL(TerrainTools, TerrainGenerate, "Generate Terrain", "grid", "", [](){})
REGISTER_EDITOR_TOOL(TerrainTools, TerrainImport,   "Import Heightmap", "open", "", [](){})

// ===== Spline mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Spline, SplineTools, "Splines", "move", 10)
REGISTER_EDITOR_TOOL(SplineTools, SplineDraw,   "Draw Spline",   "plus", "", [](){})
REGISTER_EDITOR_TOOL(SplineTools, SplineEdit,   "Edit Control Points", "move", "", [](){})

// ===== Modeling mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Modeling, ModelingOps, "Mesh Operations", "cube", 10)
REGISTER_EDITOR_TOOL(ModelingOps, ModelingExtrude, "Extrude", "plus", "", [](){})
REGISTER_EDITOR_TOOL(ModelingOps, ModelingInset,   "Inset",   "scale", "", [](){})
REGISTER_EDITOR_TOOL(ModelingOps, ModelingBevel,   "Bevel",   "build", "", [](){})
REGISTER_EDITOR_TOOL(ModelingOps, ModelingBoolean, "Boolean", "package", "", [](){})

// ===== Paint mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Paint, PaintTools, "Painting", "material", 10)
REGISTER_EDITOR_TOOL(PaintTools, PaintVertex,  "Vertex Paint",  "material", "", [](){})
REGISTER_EDITOR_TOOL(PaintTools, PaintTexture, "Texture Paint", "texture", "", [](){})

// ===== Animation mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Animation, AnimationTools, "Animation", "play", 10)
REGISTER_EDITOR_TOOL(AnimationTools, AnimRecord, "Record", "record", "", [](){})
REGISTER_EDITOR_TOOL(AnimationTools, AnimScrub,  "Scrub Timeline", "play", "", [](){})

// ===== Physics mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Physics, PhysicsTools, "Physics", "snap", 10)
REGISTER_EDITOR_TOOL(PhysicsTools, PhysicsSimulate, "Simulate", "play", "", [](){})
REGISTER_EDITOR_TOOL(PhysicsTools, PhysicsCollision, "Edit Collision", "cube", "", [](){})

// ===== Navigation mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Navigation, NavigationTools, "Navigation", "compass", 10)
REGISTER_EDITOR_TOOL(NavigationTools, NavBake,  "Build NavMesh", "grid", "", [](){})
REGISTER_EDITOR_TOOL(NavigationTools, NavPaint, "Paint Nav Area", "brush", "", [](){})

// ===== FX mode =====
REGISTER_EDITOR_TOOL_CATEGORY(FX, FXTools, "Effects", "star", 10)
REGISTER_EDITOR_TOOL(FXTools, FXSpawn, "Spawn Emitter", "star", "", [](){})
REGISTER_EDITOR_TOOL(FXTools, FXBake,  "Bake Niagara", "refresh", "", [](){})

// ===== AI mode =====
REGISTER_EDITOR_TOOL_CATEGORY(AI, AITools, "AI", "code", 10)
REGISTER_EDITOR_TOOL(AITools, AIBehaviorTree, "Behavior Tree", "hierarchy", "", [](){})
REGISTER_EDITOR_TOOL(AITools, AIBlackboard,   "Blackboard",    "properties", "", [](){})

// ===== Audio mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Audio, AudioTools, "Audio", "info", 10)
REGISTER_EDITOR_TOOL(AudioTools, AudioPlace, "Place Sound", "plus", "", [](){})
REGISTER_EDITOR_TOOL(AudioTools, AudioProbe, "Audio Probe", "search", "", [](){})

// ===== UI mode =====
REGISTER_EDITOR_TOOL_CATEGORY(UI, UITools, "UI Authoring", "menu", 10)
REGISTER_EDITOR_TOOL(UITools, UIWidget, "Widget", "menu", "", [](){})
REGISTER_EDITOR_TOOL(UITools, UILayout, "Layout Grid", "grid", "", [](){})

// ===== Lighting mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Lighting, LightingTools, "Lighting", "light", 10)
REGISTER_EDITOR_TOOL(LightingTools, LightDirectional, "Directional Light", "sun", "", [](){})
REGISTER_EDITOR_TOOL(LightingTools, LightPoint,       "Point Light",       "point-light", "", [](){})
REGISTER_EDITOR_TOOL(LightingTools, LightBuild,       "Build Lighting",    "build", "", [](){})

// ===== Cinematics mode =====
REGISTER_EDITOR_TOOL_CATEGORY(Cinematics, CinematicsTools, "Sequencer", "camera", 10)
REGISTER_EDITOR_TOOL(CinematicsTools, CineAddShot,   "Add Camera", "camera", "", [](){})
REGISTER_EDITOR_TOOL(CinematicsTools, CineKeyframe,  "Keyframe",   "record", "", [](){})

} // namespace we::programs::editor
