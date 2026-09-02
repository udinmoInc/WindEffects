#pragma warning(disable: 4505)
#include "LandscapeWorkspaceInternal.h"
#include "LandscapeFormLayout.h"
#include "KindUI/Core/WindIcon.h"

#include <algorithm>

namespace we::editor::terrain {
namespace {
using we::runtime::kindui::kWindIconNone;
namespace WindIcons = we::runtime::kindui::WindIcons;
} // namespace

void BuildCreateTab(const std::shared_ptr<we::runtime::kindui::Column>& layout, ILandscapeEditor& editor) {
    auto& dialog = editor.Dialog();
    auto& wizard = editor.Wizard();
    wizard.State() = dialog;

    AddFormSectionTitle(layout, "Generator");
    const auto selectGen = [&](runtime_terrain::TerrainGeneratorId id,
                               runtime_terrain::TerrainCreationMethod method) {
        return [&editor, id, method]() {
            auto& d = editor.Dialog();
            d.generatorId = id;
            d.creationMethod = method;
            d.generator.generator = id;
            editor.Wizard().State() = d;
        };
    };

    AddFormChipRow(layout, {
        {"Flat", WindIcons::Grid3x316, dialog.generatorId == runtime_terrain::TerrainGeneratorId::Flat
            && dialog.creationMethod != runtime_terrain::TerrainCreationMethod::HeightmapImport,
            selectGen(runtime_terrain::TerrainGeneratorId::Flat,
                runtime_terrain::TerrainCreationMethod::Flat)},
        {"Empty", WindIcons::Minus16, dialog.generatorId == runtime_terrain::TerrainGeneratorId::Empty,
            selectGen(runtime_terrain::TerrainGeneratorId::Empty,
                runtime_terrain::TerrainCreationMethod::Empty)},
        {"Perlin", kWindIconNone, dialog.generatorId == runtime_terrain::TerrainGeneratorId::PerlinNoise,
            selectGen(runtime_terrain::TerrainGeneratorId::PerlinNoise,
                runtime_terrain::TerrainCreationMethod::Noise)},
        {"FBM", kWindIconNone, dialog.generatorId == runtime_terrain::TerrainGeneratorId::Fbm,
            selectGen(runtime_terrain::TerrainGeneratorId::Fbm,
                runtime_terrain::TerrainCreationMethod::Fractal)},
        {"Ridged", kWindIconNone, dialog.generatorId == runtime_terrain::TerrainGeneratorId::RidgedNoise,
            selectGen(runtime_terrain::TerrainGeneratorId::RidgedNoise,
                runtime_terrain::TerrainCreationMethod::Fractal)},
        {"Voronoi", kWindIconNone, dialog.generatorId == runtime_terrain::TerrainGeneratorId::Voronoi,
            selectGen(runtime_terrain::TerrainGeneratorId::Voronoi,
                runtime_terrain::TerrainCreationMethod::Procedural)},
        {"Island", WindIcons::Globe16, dialog.generatorId == runtime_terrain::TerrainGeneratorId::Island,
            selectGen(runtime_terrain::TerrainGeneratorId::Island,
                runtime_terrain::TerrainCreationMethod::Procedural)},
        {"Heightmap", kWindIconNone,
            dialog.creationMethod == runtime_terrain::TerrainCreationMethod::HeightmapImport,
            [&editor]() {
                auto& d = editor.Dialog();
                d.creationMethod = runtime_terrain::TerrainCreationMethod::HeightmapImport;
                editor.Wizard().State() = d;
            }},
    });

    AddFormSectionTitle(layout, "Terrain Settings");
    AddFormField(layout, "Name", dialog.name, [&](std::string_view v) {
        editor.Dialog().name = std::string(v);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(layout, "Width", FormFormatFloat(dialog.createInfo.worldSizeX), [&](std::string_view v) {
        editor.Dialog().createInfo.worldSizeX = FormParseFloat(v, editor.Dialog().createInfo.worldSizeX);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(layout, "Height", FormFormatFloat(dialog.createInfo.worldSizeY), [&](std::string_view v) {
        editor.Dialog().createInfo.worldSizeY = FormParseFloat(v, editor.Dialog().createInfo.worldSizeY);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(layout, "Resolution X", FormFormatInt(dialog.createInfo.resolutionX), [&](std::string_view v) {
        editor.Dialog().createInfo.resolutionX = FormParseInt(v, editor.Dialog().createInfo.resolutionX);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(layout, "Resolution Y", FormFormatInt(dialog.createInfo.resolutionY), [&](std::string_view v) {
        editor.Dialog().createInfo.resolutionY = FormParseInt(v, editor.Dialog().createInfo.resolutionY);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(layout, "Chunk Size", FormFormatInt(dialog.createInfo.chunkQuads), [&](std::string_view v) {
        editor.Dialog().createInfo.chunkQuads = FormParseInt(v, editor.Dialog().createInfo.chunkQuads);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(layout, "Section Size", FormFormatInt(dialog.createInfo.tileSize), [&](std::string_view v) {
        editor.Dialog().createInfo.tileSize = FormParseInt(v, editor.Dialog().createInfo.tileSize);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(layout, "World Scale", FormFormatFloat(dialog.createInfo.worldScale.x), [&](std::string_view v) {
        const float s = FormParseFloat(v, editor.Dialog().createInfo.worldScale.x);
        editor.Dialog().createInfo.worldScale = {s, s, s};
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(layout, "Initial Height", FormFormatFloat(dialog.createInfo.initialElevation), [&](std::string_view v) {
        editor.Dialog().createInfo.initialElevation =
            std::clamp(FormParseFloat(v, editor.Dialog().createInfo.initialElevation), 0.f, 1.f);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(layout, "Material", dialog.materialSlot0.empty() ? "None" : dialog.materialSlot0,
        [&](std::string_view v) {
            editor.Dialog().materialSlot0 = (v == "None") ? "" : std::string(v);
            editor.Wizard().State() = editor.Dialog();
        });
    AddFormField(layout, "Position X", FormFormatFloat(dialog.createInfo.worldOrigin.x), [&](std::string_view v) {
        editor.Dialog().createInfo.worldOrigin.x = FormParseFloat(v, editor.Dialog().createInfo.worldOrigin.x);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(layout, "Position Y", FormFormatFloat(dialog.createInfo.worldOrigin.y), [&](std::string_view v) {
        editor.Dialog().createInfo.worldOrigin.y = FormParseFloat(v, editor.Dialog().createInfo.worldOrigin.y);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(layout, "Position Z", FormFormatFloat(dialog.createInfo.worldOrigin.z), [&](std::string_view v) {
        editor.Dialog().createInfo.worldOrigin.z = FormParseFloat(v, editor.Dialog().createInfo.worldOrigin.z);
        editor.Wizard().State() = editor.Dialog();
    });

    if (dialog.creationMethod == runtime_terrain::TerrainCreationMethod::HeightmapImport) {
        AddFormField(layout, "Heightmap Path", dialog.importHeightmapPath.string(), [&](std::string_view v) {
            editor.Dialog().importHeightmapPath = std::filesystem::path(std::string(v));
            editor.Wizard().State() = editor.Dialog();
        });
    }

    AddFormSectionTitle(layout, "Streaming");
    AddFormToggle(layout, "Enable Streaming", dialog.enableStreaming, [&]() {
        editor.Dialog().enableStreaming = !editor.Dialog().enableStreaming;
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormToggle(layout, "Enable LOD", dialog.enableLod, [&]() {
        editor.Dialog().enableLod = !editor.Dialog().enableLod;
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormToggle(layout, "Collision", dialog.enableCollision, [&]() {
        editor.Dialog().enableCollision = !editor.Dialog().enableCollision;
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormToggle(layout, "Edit Layers", dialog.enableEditLayers, [&]() {
        editor.Dialog().enableEditLayers = !editor.Dialog().enableEditLayers;
        editor.Wizard().State() = editor.Dialog();
    });
}

} // namespace we::editor::terrain
