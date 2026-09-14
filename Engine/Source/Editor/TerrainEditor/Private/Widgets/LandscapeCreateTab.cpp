// ==============================================================================
// WindEffects — TerrainEditor — LandscapeCreateTab
// UI widget used by the TerrainEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma warning(disable: 4505)
#include "LandscapeWorkspaceInternal.h"
#include "LandscapeFormLayout.h"
#include <KindUI/EditorUI.h>
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

    // 1. Generator Category
    auto genGroup = AddFormSection(layout, "Generator", true);
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

    AddFormChipRow(genGroup, {
        {"Flat", WindIcons::Grid16, dialog.generatorId == runtime_terrain::TerrainGeneratorId::Flat
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

    // 2. Terrain Settings Category
    auto settingsGroup = AddFormSection(layout, "Terrain Settings", true);
    AddFormField(settingsGroup, "Name", dialog.name, [&](std::string_view v) {
        editor.Dialog().name = std::string(v);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(settingsGroup, "Width", FormFormatFloat(dialog.createInfo.worldSizeX), [&](std::string_view v) {
        editor.Dialog().createInfo.worldSizeX = FormParseFloat(v, editor.Dialog().createInfo.worldSizeX);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(settingsGroup, "Height", FormFormatFloat(dialog.createInfo.worldSizeY), [&](std::string_view v) {
        editor.Dialog().createInfo.worldSizeY = FormParseFloat(v, editor.Dialog().createInfo.worldSizeY);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(settingsGroup, "Resolution X", FormFormatInt(dialog.createInfo.resolutionX), [&](std::string_view v) {
        editor.Dialog().createInfo.resolutionX = FormParseInt(v, editor.Dialog().createInfo.resolutionX);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(settingsGroup, "Resolution Y", FormFormatInt(dialog.createInfo.resolutionY), [&](std::string_view v) {
        editor.Dialog().createInfo.resolutionY = FormParseInt(v, editor.Dialog().createInfo.resolutionY);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(settingsGroup, "Chunk Size", FormFormatInt(dialog.createInfo.chunkQuads), [&](std::string_view v) {
        editor.Dialog().createInfo.chunkQuads = FormParseInt(v, editor.Dialog().createInfo.chunkQuads);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(settingsGroup, "Section Size", FormFormatInt(dialog.createInfo.tileSize), [&](std::string_view v) {
        editor.Dialog().createInfo.tileSize = FormParseInt(v, editor.Dialog().createInfo.tileSize);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(settingsGroup, "World Scale", FormFormatFloat(dialog.createInfo.worldScale.x), [&](std::string_view v) {
        const float s = FormParseFloat(v, editor.Dialog().createInfo.worldScale.x);
        editor.Dialog().createInfo.worldScale = {s, s, s};
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(settingsGroup, "Initial Height", FormFormatFloat(dialog.createInfo.initialElevation),
        [&](std::string_view v) {
        editor.Dialog().createInfo.initialElevation =
            std::clamp(FormParseFloat(v, editor.Dialog().createInfo.initialElevation), 0.f, 1.f);
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormField(settingsGroup, "Material", dialog.materialSlot0.empty() ? "None" : dialog.materialSlot0,
        [&](std::string_view v) {
            editor.Dialog().materialSlot0 = (v == "None") ? "" : std::string(v);
            editor.Wizard().State() = editor.Dialog();
        });

    if (dialog.creationMethod == runtime_terrain::TerrainCreationMethod::HeightmapImport) {
        AddFormField(settingsGroup, "Heightmap Path", dialog.importHeightmapPath.string(), [&](std::string_view v) {
            editor.Dialog().importHeightmapPath = std::filesystem::path(std::string(v));
            editor.Wizard().State() = editor.Dialog();
        });
    }

    // 3. Transform Category (Position X, Y, Z combined into Inspector Vector3 row)
    auto transformGroup = AddFormSection(layout, "Transform", true);
    AddFormVector3Field(transformGroup, "Position",
        dialog.createInfo.worldOrigin.x,
        dialog.createInfo.worldOrigin.y,
        dialog.createInfo.worldOrigin.z,
        [&](float x, float y, float z) {
            editor.Dialog().createInfo.worldOrigin = { x, y, z };
            editor.Wizard().State() = editor.Dialog();
        });

    // 4. Streaming & Options Category (Toggles as Inspector CheckBox rows)
    auto streamingGroup = AddFormSection(layout, "Streaming", true);
    AddFormToggle(streamingGroup, "Enable Streaming", dialog.enableStreaming, [&](bool val) {
        editor.Dialog().enableStreaming = val;
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormToggle(streamingGroup, "Enable LOD", dialog.enableLod, [&](bool val) {
        editor.Dialog().enableLod = val;
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormToggle(streamingGroup, "Collision", dialog.enableCollision, [&](bool val) {
        editor.Dialog().enableCollision = val;
        editor.Wizard().State() = editor.Dialog();
    });
    AddFormToggle(streamingGroup, "Edit Layers", dialog.enableEditLayers, [&](bool val) {
        editor.Dialog().enableEditLayers = val;
        editor.Wizard().State() = editor.Dialog();
    });
}

} // namespace we::editor::terrain
