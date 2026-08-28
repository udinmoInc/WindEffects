#pragma warning(disable: 4505)
#include "LandscapeWorkspaceInternal.h"
#include "KindUI/Widgets/Components.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"

#include "LandscapePanelChrome.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace we::editor::terrain {
namespace {

using namespace we::runtime::kindui;

void AddSectionTitle(const std::shared_ptr<Column>& layout, std::string_view title) {
    auto header = MakeSectionHeader(std::string(title));
    layout->AddChild(header);
}

void AddChipRow(
    const std::shared_ptr<Column>& layout,
    const std::vector<std::tuple<std::string, const char*, bool, std::function<void()>>>& chips)
{
    const size_t maxPerRow = 4;
    std::shared_ptr<Row> currentRow = nullptr;
    size_t countInRow = 0;

    for (const auto& [label, icon, selected, onClick] : chips) {
        if (!currentRow || countInRow >= maxPerRow) {
            currentRow = MakeRow();
            currentRow->Gap(4.0f);
            currentRow->SetFlexShrink(0.0f);
            layout->AddChild(currentRow);
            countInRow = 0;
        }
        auto btn = MakeSecondaryAction(label, icon ? icon : "");
        btn->SetFlexGrow(1.0f);
        btn->SetFlexShrink(1.0f);
        btn->SetMinWidth(50.0f);
        btn->SetOnClicked(onClick);
        currentRow->AddChild(btn);
        ++countInRow;
    }
}

void AddField(
    const std::shared_ptr<Column>& layout,
    std::string label,
    std::string value,
    std::function<void(std::string_view)> onCommit)
{
    auto row = MakeRow();
    row->Align(AlignItems::Center);
    row->Gap(8.0f);
    auto lbl = std::make_shared<Label>(label);
    lbl->SetMinWidth(120.0f);
    lbl->SetMaxWidth(120.0f);
    lbl->SetFlexShrink(0.0f);
    lbl->SetFlexGrow(0.0f);

    auto input = std::make_shared<SearchBoxControl>("");
    input->SetText(value);
    input->SetFlexGrow(1.0f);
    input->SetFlexShrink(1.0f);
    input->SetMinWidth(60.0f);
    input->SetOnChanged([onCommit](const std::string& v) { onCommit(v); });

    row->AddChild(lbl);
    row->AddChild(input);
    layout->AddChild(row);
}

void AddToggle(
    const std::shared_ptr<Column>& layout,
    std::string label,
    bool on,
    std::function<void()> onClick)
{
    auto btn = MakeSecondaryAction(label + (on ? " : ON" : " : OFF"));
    btn->SetOnClicked(onClick);
    layout->AddChild(btn);
}

void AddButton(
    const std::shared_ptr<Column>& layout,
    std::string label,
    std::function<void()> onClick,
    bool danger = false)
{
    std::shared_ptr<DesignButton> btn; if(danger) btn = MakePrimaryAction(label); else btn = MakeSecondaryAction(label);
    // If it's danger, maybe we don't have MakeDangerAction, just use Primary
    btn->SetOnClicked(onClick);
    layout->AddChild(btn);
}


std::string FormatFloat(float v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.3g", static_cast<double>(v));
    return buf;
}

std::string FormatInt(int v) {
    return std::to_string(v);
}

float ParseFloat(std::string_view s, float fallback) {
    try {
        return std::stof(std::string(s));
    } catch (...) {
        return fallback;
    }
}

int ParseInt(std::string_view s, int fallback) {
    try {
        return std::stoi(std::string(s));
    } catch (...) {
        return fallback;
    }
}
} // namespace


void BuildCreateTab(const std::shared_ptr<we::runtime::kindui::Column>& layout, ILandscapeEditor& editor) {
    auto& dialog = editor.Dialog();
    auto& wizard = editor.Wizard();
    wizard.State() = dialog;

    AddSectionTitle(layout, "Generator");
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

    AddChipRow(layout, {
        {"Flat", "grid", dialog.generatorId == runtime_terrain::TerrainGeneratorId::Flat
            && dialog.creationMethod != runtime_terrain::TerrainCreationMethod::HeightmapImport,
            selectGen(runtime_terrain::TerrainGeneratorId::Flat,
                runtime_terrain::TerrainCreationMethod::Flat)},
        {"Empty", "minus", dialog.generatorId == runtime_terrain::TerrainGeneratorId::Empty,
            selectGen(runtime_terrain::TerrainGeneratorId::Empty,
                runtime_terrain::TerrainCreationMethod::Empty)},
        {"Perlin", "waves", dialog.generatorId == runtime_terrain::TerrainGeneratorId::PerlinNoise,
            selectGen(runtime_terrain::TerrainGeneratorId::PerlinNoise,
                runtime_terrain::TerrainCreationMethod::Noise)},
        {"FBM", "layers", dialog.generatorId == runtime_terrain::TerrainGeneratorId::Fbm,
            selectGen(runtime_terrain::TerrainGeneratorId::Fbm,
                runtime_terrain::TerrainCreationMethod::Fractal)},
        {"Ridged", "mountain", dialog.generatorId == runtime_terrain::TerrainGeneratorId::RidgedNoise,
            selectGen(runtime_terrain::TerrainGeneratorId::RidgedNoise,
                runtime_terrain::TerrainCreationMethod::Fractal)},
        {"Voronoi", "hexagon", dialog.generatorId == runtime_terrain::TerrainGeneratorId::Voronoi,
            selectGen(runtime_terrain::TerrainGeneratorId::Voronoi,
                runtime_terrain::TerrainCreationMethod::Procedural)},
        {"Island", "globe", dialog.generatorId == runtime_terrain::TerrainGeneratorId::Island,
            selectGen(runtime_terrain::TerrainGeneratorId::Island,
                runtime_terrain::TerrainCreationMethod::Procedural)},
        {"Heightmap", "image",
            dialog.creationMethod == runtime_terrain::TerrainCreationMethod::HeightmapImport,
            [&editor]() {
                auto& d = editor.Dialog();
                d.creationMethod = runtime_terrain::TerrainCreationMethod::HeightmapImport;
                editor.Wizard().State() = d;
            }},
    });

    AddSectionTitle(layout, "Terrain Settings");
    AddField(layout, "Name", dialog.name, [&](std::string_view v) {
        editor.Dialog().name = std::string(v);
        editor.Wizard().State() = editor.Dialog();
    });
    AddField(layout, "Width", FormatFloat(dialog.createInfo.worldSizeX), [&](std::string_view v) {
        editor.Dialog().createInfo.worldSizeX = ParseFloat(v, editor.Dialog().createInfo.worldSizeX);
        editor.Wizard().State() = editor.Dialog();
    });
    AddField(layout, "Height", FormatFloat(dialog.createInfo.worldSizeY), [&](std::string_view v) {
        editor.Dialog().createInfo.worldSizeY = ParseFloat(v, editor.Dialog().createInfo.worldSizeY);
        editor.Wizard().State() = editor.Dialog();
    });
    AddField(layout, "Resolution X", FormatInt(dialog.createInfo.resolutionX), [&](std::string_view v) {
        editor.Dialog().createInfo.resolutionX = ParseInt(v, editor.Dialog().createInfo.resolutionX);
        editor.Wizard().State() = editor.Dialog();
    });
    AddField(layout, "Resolution Y", FormatInt(dialog.createInfo.resolutionY), [&](std::string_view v) {
        editor.Dialog().createInfo.resolutionY = ParseInt(v, editor.Dialog().createInfo.resolutionY);
        editor.Wizard().State() = editor.Dialog();
    });
    AddField(layout, "Chunk Size", FormatInt(dialog.createInfo.chunkQuads), [&](std::string_view v) {
        editor.Dialog().createInfo.chunkQuads = ParseInt(v, editor.Dialog().createInfo.chunkQuads);
        editor.Wizard().State() = editor.Dialog();
    });
    AddField(layout, "Section Size", FormatInt(dialog.createInfo.tileSize), [&](std::string_view v) {
        editor.Dialog().createInfo.tileSize = ParseInt(v, editor.Dialog().createInfo.tileSize);
        editor.Wizard().State() = editor.Dialog();
    });
    AddField(layout, "World Scale", FormatFloat(dialog.createInfo.worldScale.x), [&](std::string_view v) {
        const float s = ParseFloat(v, editor.Dialog().createInfo.worldScale.x);
        editor.Dialog().createInfo.worldScale = {s, s, s};
        editor.Wizard().State() = editor.Dialog();
    });
    AddField(layout, "Initial Height", FormatFloat(dialog.createInfo.initialElevation), [&](std::string_view v) {
        editor.Dialog().createInfo.initialElevation =
            std::clamp(ParseFloat(v, editor.Dialog().createInfo.initialElevation), 0.f, 1.f);
        editor.Wizard().State() = editor.Dialog();
    });
    AddField(layout, "Material", dialog.materialSlot0.empty() ? "None" : dialog.materialSlot0,
        [&](std::string_view v) {
            editor.Dialog().materialSlot0 = (v == "None") ? "" : std::string(v);
            editor.Wizard().State() = editor.Dialog();
        });
    AddField(layout, "Position X", FormatFloat(dialog.createInfo.worldOrigin.x), [&](std::string_view v) {
        editor.Dialog().createInfo.worldOrigin.x = ParseFloat(v, editor.Dialog().createInfo.worldOrigin.x);
        editor.Wizard().State() = editor.Dialog();
    });
    AddField(layout, "Position Y", FormatFloat(dialog.createInfo.worldOrigin.y), [&](std::string_view v) {
        editor.Dialog().createInfo.worldOrigin.y = ParseFloat(v, editor.Dialog().createInfo.worldOrigin.y);
        editor.Wizard().State() = editor.Dialog();
    });
    AddField(layout, "Position Z", FormatFloat(dialog.createInfo.worldOrigin.z), [&](std::string_view v) {
        editor.Dialog().createInfo.worldOrigin.z = ParseFloat(v, editor.Dialog().createInfo.worldOrigin.z);
        editor.Wizard().State() = editor.Dialog();
    });

    if (dialog.creationMethod == runtime_terrain::TerrainCreationMethod::HeightmapImport) {
        AddField(layout, "Heightmap Path", dialog.importHeightmapPath.string(), [&](std::string_view v) {
            editor.Dialog().importHeightmapPath = std::filesystem::path(std::string(v));
            editor.Wizard().State() = editor.Dialog();
        });
    }

    AddSectionTitle(layout, "Streaming");
    AddToggle(layout, "Enable Streaming", dialog.enableStreaming, [&]() {
        editor.Dialog().enableStreaming = !editor.Dialog().enableStreaming;
        editor.Wizard().State() = editor.Dialog();
    });
    AddToggle(layout, "Enable LOD", dialog.enableLod, [&]() {
        editor.Dialog().enableLod = !editor.Dialog().enableLod;
        editor.Wizard().State() = editor.Dialog();
    });
    AddToggle(layout, "Collision", dialog.enableCollision, [&]() {
        editor.Dialog().enableCollision = !editor.Dialog().enableCollision;
        editor.Wizard().State() = editor.Dialog();
    });
    AddToggle(layout, "Edit Layers", dialog.enableEditLayers, [&]() {
        editor.Dialog().enableEditLayers = !editor.Dialog().enableEditLayers;
        editor.Wizard().State() = editor.Dialog();
    });
}

} // namespace we::editor::terrain

