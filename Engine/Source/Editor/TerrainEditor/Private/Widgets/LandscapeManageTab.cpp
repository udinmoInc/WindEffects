#include "LandscapeWorkspaceInternal.h"
#include "LandscapePanelChrome.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace we::editor::terrain {
namespace {

namespace Chrome = LandscapePanelChrome;

void AddSectionTitle(LandscapeLayoutBuilder& layout, std::string_view title) {
    LandscapeHitTarget t{};
    t.kind = LandscapeHitKind::None;
    t.geometry = {layout.contentX, layout.cursorY, layout.contentW, Chrome::RowHeight()};
    t.label = std::string(title);
    t.id = -1;
    layout.targets.push_back(std::move(t));
    layout.Advance(Chrome::RowHeight() + 4.f);
}

void AddInfo(LandscapeLayoutBuilder& layout, std::string label, std::string value) {
    LandscapeHitTarget t{};
    t.kind = LandscapeHitKind::None;
    t.geometry = {layout.contentX, layout.cursorY, layout.contentW, Chrome::RowHeight()};
    t.label = std::move(label);
    t.value = std::move(value);
    t.id = -2; // info row
    layout.targets.push_back(std::move(t));
    layout.Advance(Chrome::RowHeight() + 2.f);
}

void AddField(
    LandscapeLayoutBuilder& layout,
    std::string label,
    std::string value,
    std::function<void(std::string_view)> onCommit)
{
    const float h = Chrome::RowHeight();
    LandscapeHitTarget t{};
    t.kind = LandscapeHitKind::Field;
    t.geometry = {
        layout.contentX + Chrome::LabelColumnWidth() + 8.f,
        layout.cursorY,
        layout.contentW - Chrome::LabelColumnWidth() - 8.f,
        h};
    t.label = std::move(label);
    t.value = std::move(value);
    t.editable = true;
    t.onTextCommit = std::move(onCommit);
    layout.targets.push_back(std::move(t));
    layout.Advance(h + 4.f);
}

void AddButton(
    LandscapeLayoutBuilder& layout,
    std::string label,
    std::function<void()> onClick,
    bool danger = false)
{
    LandscapeHitTarget t{};
    t.kind = LandscapeHitKind::Button;
    t.geometry = {layout.contentX, layout.cursorY, layout.contentW, Chrome::RowHeight() + 4.f};
    t.label = std::move(label);
    t.onClick = std::move(onClick);
    t.isDanger = danger;
    layout.targets.push_back(std::move(t));
    layout.Advance(Chrome::RowHeight() + 10.f);
}

std::string FormatBytes(std::uint64_t bytes) {
    if (bytes < 1024) {
        return std::to_string(bytes) + " B";
    }
    if (bytes < 1024ull * 1024ull) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
        return buf;
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.2f MB", bytes / (1024.0 * 1024.0));
    return buf;
}

int ParseInt(std::string_view s, int fallback) {
    try {
        return std::stoi(std::string(s));
    } catch (...) {
        return fallback;
    }
}

} // namespace

void BuildManageTab(
    LandscapeLayoutBuilder& layout,
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
