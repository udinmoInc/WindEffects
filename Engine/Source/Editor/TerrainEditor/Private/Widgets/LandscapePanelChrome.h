#pragma once

#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Geometry.h"

#include <string>
#include <string_view>

namespace we::editor::terrain::LandscapePanelChrome {

float PanelPad();
float SectionGap();
float RowHeight();
float ChipHeight();
float TabBarHeight();
float PrimaryButtonHeight();
float LabelColumnWidth();

void PaintPanelBackground(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds);

void PaintTabBar(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds);

void PaintTab(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds,
    std::string_view label,
    bool active,
    float hoverAnim);

void PaintSectionCard(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds);

void PaintSectionTitle(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds,
    std::string_view title);

void PaintSoftSeparator(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds);

void PaintChip(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds,
    std::string_view label,
    const char* iconHook,
    bool selected,
    float hoverAnim);

void PaintPropertyLabel(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds,
    std::string_view label);

void PaintField(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds,
    std::string_view value,
    bool focused,
    bool hovered);

void PaintToggle(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds,
    std::string_view label,
    bool on,
    float hoverAnim);

void PaintPrimaryButton(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds,
    std::string_view label,
    float hoverAnim,
    float pressAnim);

void PaintSecondaryButton(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds,
    std::string_view label,
    float hoverAnim,
    float pressAnim);

void PaintDangerButton(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds,
    std::string_view label,
    float hoverAnim,
    float pressAnim);

void PaintInfoValue(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds,
    std::string_view label,
    std::string_view value);

} // namespace we::editor::terrain::LandscapePanelChrome
