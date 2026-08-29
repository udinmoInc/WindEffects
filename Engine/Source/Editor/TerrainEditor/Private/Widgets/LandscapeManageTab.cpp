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

static void AddSectionTitle(const std::shared_ptr<Column>& layout, std::string_view title) {
    auto header = MakeSectionHeader(std::string(title));
    layout->AddChild(header);
}

static void AddChipRow(
    const std::shared_ptr<Column>& layout,
    const std::vector<std::tuple<std::string, const char*, bool, std::function<void()>>>& chips)
{
    const size_t maxPerRow = 4;
    std::shared_ptr<Row> currentRow = nullptr;
    size_t countInRow = 0;

    for (const auto& [label, icon, selected, onClick] : chips) {
        if (!currentRow || countInRow >= maxPerRow) {
            currentRow = MakeRow();
            currentRow->Gap(we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space1));
            currentRow->SetFlexShrink(0.0f);
            layout->AddChild(currentRow);
            countInRow = 0;
        }
        auto btn = MakeSecondaryAction(label, icon ? icon : "");
        btn->SetFlexGrow(1.0f);
        btn->SetFlexShrink(1.0f);
        btn->SetMinWidth(ChipButtonMinWidth());
        btn->SetOnClicked(onClick);
        currentRow->AddChild(btn);
        ++countInRow;
    }
}

static void AddField(
    const std::shared_ptr<Column>& layout,
    std::string label,
    std::string value,
    std::function<void(std::string_view)> onCommit)
{
    AddFormField(layout, label, value, std::move(onCommit));
}

static void AddToggle(
    const std::shared_ptr<Column>& layout,
    std::string label,
    bool on,
    std::function<void()> onClick)
{
    auto btn = MakeSecondaryAction(label + (on ? " : ON" : " : OFF"));
    btn->SetOnClicked(onClick);
    layout->AddChild(btn);
}

static void AddButton(
    const std::shared_ptr<Column>& layout,
    std::string label,
    std::function<void()> onClick,
    bool danger = false)
{
    std::shared_ptr<DesignButton> btn; if(danger) btn = MakePrimaryAction(label); else btn = MakeSecondaryAction(label);
    btn->SetOnClicked(onClick);
    layout->AddChild(btn);
}

static void AddLayerRow(
    const std::shared_ptr<Column>& layout,
    int layerIndex,
    std::string name,
    bool selected,
    bool visible,
    std::function<void()> onSelect,
    std::function<void()> onToggleVisibility)
{
    auto row = MakeRow();
    row->Align(AlignItems::Center);
    row->Gap(8.0f);
    
    auto visBtn = std::make_shared<IconButton>(visible ? "eye" : "eye-off");
    visBtn->SetOnClicked(onToggleVisibility);
    
    auto selectBtn = MakeSecondaryAction(name);
    selectBtn->SetOnClicked(onSelect);
    
    row->AddChild(visBtn);
    row->AddChild(selectBtn);
    layout->AddChild(row);
}

std::string FormatFloat(float v) { char buf[64]; std::snprintf(buf, sizeof(buf), "%.3g", static_cast<double>(v)); return buf; }
std::string FormatInt(int v) { return std::to_string(v); }
float ParseFloat(std::string_view s, float fallback) { try { return std::stof(std::string(s)); } catch (...) { return fallback; } }
int ParseInt(std::string_view s, int fallback) { try { return std::stoi(std::string(s)); } catch (...) { return fallback; } }

} // namespace


static void AddInfo(const std::shared_ptr<Column>& layout, std::string label, std::string value) {
    auto row = MakeRow();
    row->Align(AlignItems::Center);
    row->Gap(8.0f);
    auto lbl = std::make_shared<Label>(label);
    lbl->SetMinWidth(120.0f);
    lbl->SetMaxWidth(120.0f);
    lbl->SetFlexShrink(0.0f);
    lbl->SetFlexGrow(0.0f);

    auto val = std::make_shared<Label>(value);
    val->SetFlexGrow(1.0f);
    val->SetFlexShrink(1.0f);
    val->SetMinWidth(60.0f);

    row->AddChild(lbl);
    row->AddChild(val);
    layout->AddChild(row);
}

static std::string FormatBytes(size_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
    return std::to_string(bytes / (1024 * 1024)) + " MB";
}

void BuildManageTab(
    const std::shared_ptr<we::runtime::kindui::Column>& layout,
    ILandscapeEditor& editor,
    std::string& importPath,
    std::string& exportPath,
    int& resizeX,
    int& resizeY)
{
    const auto info = editor.LandscapeInfo();
    AddSectionTitle(layout, "Landscape Information");
    if (!info.exists) {
        AddInfo(layout, "Status", "No Landscape");
        return;
    }

    AddInfo(layout, "Name", info.name);
    AddInfo(layout, "Resolution",
        std::to_string(info.resolutionX) + " x " + std::to_string(info.resolutionY));
    AddInfo(layout, "Chunks",
        std::to_string(info.chunkCountX) + " x " + std::to_string(info.chunkCountZ)
            + " (" + std::to_string(info.chunkQuads) + " quads)");
    AddInfo(layout, "LOD", info.lodEnabled
        ? ("Enabled (max " + std::to_string(info.maxLod) + ")")
        : "Disabled");
    AddInfo(layout, "Streaming", info.streamingEnabled ? "Enabled" : "Disabled");
    AddInfo(layout, "Memory", FormatBytes(info.sampleMemoryBytes));
    AddInfo(layout, "Material", info.materialSlot0.empty() ? "None" : info.materialSlot0);
    AddInfo(layout, "Collision", info.collisionEnabled ? "Enabled" : "Disabled");

    AddSectionTitle(layout, "Actions");
    AddField(layout, "Resize X", std::to_string(resizeX), [&](std::string_view v) {
        resizeX = ParseInt(v, resizeX);
    });
    AddField(layout, "Resize Y", std::to_string(resizeY), [&](std::string_view v) {
        resizeY = ParseInt(v, resizeY);
    });
    AddButton(layout, "Resize", [&]() {
        (void)editor.ResizeLandscape(resizeX, resizeY);
    });
    AddButton(layout, "Frame Landscape", [&]() { editor.FrameLandscape(); });
    AddButton(layout, "Rebuild", [&]() { (void)editor.RebuildMeshes(); });
    AddButton(layout, "Generate Collision", [&]() { (void)editor.RebuildCollision(); });
    AddButton(layout, "Rebuild LOD", [&]() { (void)editor.RebuildLOD(); });

    AddField(layout, "Import Path", importPath, [&](std::string_view v) {
        importPath = std::string(v);
    });
    AddButton(layout, "Import Heightmap", [&]() {
        if (!importPath.empty()) {
            (void)editor.ImportHeightmap(importPath);
        }
    });
    AddField(layout, "Export Path", exportPath, [&](std::string_view v) {
        exportPath = std::string(v);
    });
    AddButton(layout, "Export Heightmap", [&]() {
        if (!exportPath.empty()) {
            (void)editor.ExportHeightmap(exportPath);
        }
    });
    AddButton(layout, "Delete Landscape", [&]() { (void)editor.DeleteLandscape(); }, true);
}

} // namespace we::editor::terrain

