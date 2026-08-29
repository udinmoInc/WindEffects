#pragma once

#include "KindUI/Layout/Flex.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace we::editor::terrain {

using FormChip = std::tuple<std::string, const char*, bool, std::function<void()>>;

void ConfigureLandscapeFormColumn(const std::shared_ptr<we::runtime::kindui::Column>& layout);

void AddFormSectionTitle(
    const std::shared_ptr<we::runtime::kindui::Column>& layout,
    std::string_view title);

void AddFormField(
    const std::shared_ptr<we::runtime::kindui::Column>& layout,
    const std::string& label,
    const std::string& value,
    std::function<void(std::string_view)> onCommit);

void AddFormChipRow(
    const std::shared_ptr<we::runtime::kindui::Column>& layout,
    const std::vector<FormChip>& chips,
    size_t maxPerRow = 4);

void AddFormToggle(
    const std::shared_ptr<we::runtime::kindui::Column>& layout,
    const std::string& label,
    bool on,
    std::function<void()> onClick);

void AddFormButton(
    const std::shared_ptr<we::runtime::kindui::Column>& layout,
    const std::string& label,
    std::function<void()> onClick,
    bool primary = false);

void AddFormInfoRow(
    const std::shared_ptr<we::runtime::kindui::Column>& layout,
    const std::string& label,
    const std::string& value);

[[nodiscard]] std::string FormFormatFloat(float value);
[[nodiscard]] std::string FormFormatInt(int value);
[[nodiscard]] float FormParseFloat(std::string_view text, float fallback);
[[nodiscard]] int FormParseInt(std::string_view text, int fallback);

} // namespace we::editor::terrain
