#pragma warning(disable: 4505)
#include "LandscapeWorkspaceInternal.h"
#include "LandscapeFormLayout.h"

#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Widgets/Components.h"

namespace we::editor::terrain {
namespace {

using namespace we::runtime::kindui;

std::string FormatBytes(size_t bytes) {
    if (bytes < 1024) {
        return std::to_string(bytes) + " B";
    }
    if (bytes < 1024 * 1024) {
        return std::to_string(bytes / 1024) + " KB";
    }
    return std::to_string(bytes / (1024 * 1024)) + " MB";
}

} // namespace

void BuildManageTab(
    const std::shared_ptr<Column>& layout,
    ILandscapeEditor& editor,
    std::string& importPath,
    std::string& exportPath,
    int& resizeX,
    int& resizeY)
{
    const auto info = editor.LandscapeInfo();
    AddFormSectionTitle(layout, "Landscape Information");
    if (!info.exists) {
        AddFormInfoRow(layout, "Status", "No Landscape");
        return;
    }

    AddFormInfoRow(layout, "Name", info.name);
    AddFormInfoRow(layout, "Resolution",
        std::to_string(info.resolutionX) + " x " + std::to_string(info.resolutionY));
    AddFormInfoRow(layout, "Chunks",
        std::to_string(info.chunkCountX) + " x " + std::to_string(info.chunkCountZ)
            + " (" + std::to_string(info.chunkQuads) + " quads)");
    AddFormInfoRow(layout, "LOD", info.lodEnabled
        ? ("Enabled (max " + std::to_string(info.maxLod) + ")")
        : "Disabled");
    AddFormInfoRow(layout, "Streaming", info.streamingEnabled ? "Enabled" : "Disabled");
    AddFormInfoRow(layout, "Memory", FormatBytes(info.sampleMemoryBytes));
    AddFormInfoRow(layout, "Material", info.materialSlot0.empty() ? "None" : info.materialSlot0);
    AddFormInfoRow(layout, "Collision", info.collisionEnabled ? "Enabled" : "Disabled");

    AddFormSectionTitle(layout, "Actions");
    AddFormField(layout, "Resize X", std::to_string(resizeX), [&](std::string_view v) {
        resizeX = FormParseInt(v, resizeX);
    });
    AddFormField(layout, "Resize Y", std::to_string(resizeY), [&](std::string_view v) {
        resizeY = FormParseInt(v, resizeY);
    });
    AddFormButton(layout, "Resize", [&]() {
        (void)editor.ResizeLandscape(resizeX, resizeY);
    });
    AddFormButton(layout, "Frame Landscape", [&]() { editor.FrameLandscape(); });
    AddFormButton(layout, "Rebuild", [&]() { (void)editor.RebuildMeshes(); });
    AddFormButton(layout, "Generate Collision", [&]() { (void)editor.RebuildCollision(); });
    AddFormButton(layout, "Rebuild LOD", [&]() { (void)editor.RebuildLOD(); });

    AddFormField(layout, "Import Path", importPath, [&](std::string_view v) {
        importPath = std::string(v);
    });
    AddFormButton(layout, "Import Heightmap", [&]() {
        if (!importPath.empty()) {
            (void)editor.ImportHeightmap(importPath);
        }
    });
    AddFormField(layout, "Export Path", exportPath, [&](std::string_view v) {
        exportPath = std::string(v);
    });
    AddFormButton(layout, "Export Heightmap", [&]() {
        if (!exportPath.empty()) {
            (void)editor.ExportHeightmap(exportPath);
        }
    });
    AddFormButton(layout, "Delete Landscape", [&]() { (void)editor.DeleteLandscape(); }, true);
}

} // namespace we::editor::terrain
