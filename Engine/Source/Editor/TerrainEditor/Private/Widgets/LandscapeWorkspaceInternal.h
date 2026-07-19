#pragma once

#include "TerrainEditor/ILandscapeEditor.h"
#include "KindUI/Core/Geometry.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace we::editor::terrain {

enum class LandscapeHitKind : std::uint8_t {
    None = 0,
    Tab,
    Chip,
    Field,
    Toggle,
    Button,
    LayerRow,
};

struct LandscapeHitTarget {
    LandscapeHitKind kind = LandscapeHitKind::None;
    we::runtime::kindui::Rect geometry{};
    int id = 0;
    int subId = 0;
    std::string label;
    std::function<void()> onClick;
    std::function<void(std::string_view)> onTextCommit;
    bool selected = false;
    bool toggled = false;
    bool isDanger = false;
    bool isPrimary = false;
    bool editable = false;
    std::string value;
    const char* iconHook = nullptr;
    float hoverAnim = 0.f;
    float pressAnim = 0.f;
};

struct LandscapeLayoutBuilder {
    std::vector<LandscapeHitTarget> targets;
    float contentHeight = 0.f;
    float cursorY = 0.f;
    float contentX = 0.f;
    float contentW = 0.f;

    void Reset(float x, float y, float width) {
        targets.clear();
        contentHeight = 0.f;
        cursorY = y;
        contentX = x;
        contentW = width;
    }

    void Advance(float dy) {
        cursorY += dy;
        contentHeight = std::max(contentHeight, cursorY);
    }
};

void BuildCreateTab(LandscapeLayoutBuilder& layout, ILandscapeEditor& editor);
void BuildSculptTab(LandscapeLayoutBuilder& layout, ILandscapeEditor& editor);
void BuildPaintTab(LandscapeLayoutBuilder& layout, ILandscapeEditor& editor);
void BuildManageTab(
    LandscapeLayoutBuilder& layout,
    ILandscapeEditor& editor,
    std::string& importPath,
    std::string& exportPath,
    int& resizeX,
    int& resizeY);

} // namespace we::editor::terrain
