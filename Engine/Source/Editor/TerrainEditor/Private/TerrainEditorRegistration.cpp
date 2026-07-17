#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"
#include "TerrainEditor/TerrainEditorService.h"
#include "Terrain/TerrainTypes.h"

namespace we::programs::editor {

using we::editor::terrain::TerrainEditorService;
using we::runtime::terrain::TerrainBrushOp;

// File-scope registrations override the empty stubs in DefaultEditorModes.
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptRaise, "Raise", "plus", "", []() {
    TerrainEditorService::Get().SetBrushOp(TerrainBrushOp::Raise);
    TerrainEditorService::Get().EnsureLandscape();
})
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptLower, "Lower", "minus", "", []() {
    TerrainEditorService::Get().SetBrushOp(TerrainBrushOp::Lower);
    TerrainEditorService::Get().EnsureLandscape();
})
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptSmooth, "Smooth", "refresh", "", []() {
    TerrainEditorService::Get().SetBrushOp(TerrainBrushOp::Smooth);
    TerrainEditorService::Get().EnsureLandscape();
})
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptFlatten, "Flatten", "plane", "", []() {
    TerrainEditorService::Get().SetBrushOp(TerrainBrushOp::Flatten);
    TerrainEditorService::Get().EnsureLandscape();
})

REGISTER_EDITOR_TOOL_CATEGORY(Landscape, LandscapeErosion, "Erosion", "layers", 20)
REGISTER_EDITOR_TOOL(LandscapeErosion, SculptNoise, "Noise", "star", "", []() {
    TerrainEditorService::Get().SetBrushOp(TerrainBrushOp::Noise);
    TerrainEditorService::Get().EnsureLandscape();
})
REGISTER_EDITOR_TOOL(LandscapeErosion, SculptHydraulic, "Hydraulic Erosion", "refresh", "", []() {
    TerrainEditorService::Get().SetBrushOp(TerrainBrushOp::HydraulicErosion);
    TerrainEditorService::Get().EnsureLandscape();
})
REGISTER_EDITOR_TOOL(LandscapeErosion, SculptThermal, "Thermal Erosion", "snap", "", []() {
    TerrainEditorService::Get().SetBrushOp(TerrainBrushOp::ThermalErosion);
    TerrainEditorService::Get().EnsureLandscape();
})

REGISTER_EDITOR_TOOL(TerrainTools, TerrainGenerate, "Generate Terrain", "grid", "", []() {
    TerrainEditorService::Get().GenerateDefaultLandscape();
})
REGISTER_EDITOR_TOOL(TerrainTools, TerrainImport, "Import Heightmap", "open", "", []() {
    TerrainEditorService::Get().EnsureLandscape();
})

REGISTER_EDITOR_TOOL(FoliagePaint, FoliagePaintTool, "Paint Foliage", "layers", "Shift+4", []() {
    TerrainEditorService::Get().SpawnFoliageInDefaultRegion();
})

} // namespace we::programs::editor
