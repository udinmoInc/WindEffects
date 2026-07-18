#pragma once

#include "Model/WeProjectDescriptor.h"
#include "UI/Shell/LauncherHelpers.h"

#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/ScrollLayout.h"
#include "KindUI/Widgets/Label.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::programs::welauncher {

inline std::shared_ptr<we::runtime::kindui::ScrollLayout> FindScrollById(
    const std::shared_ptr<we::runtime::kindui::Widget>& root,
    std::string_view id) {
    if (!root) {
        return nullptr;
    }
    if (root->GetId() == id) {
        return std::dynamic_pointer_cast<we::runtime::kindui::ScrollLayout>(root);
    }
    for (const auto& child : root->GetChildren()) {
        if (auto found = FindScrollById(child, id)) {
            return found;
        }
    }
    return nullptr;
}

inline std::shared_ptr<we::runtime::kindui::Label> MakeLabel(
    const std::string& text,
    float size,
    we::runtime::kindui::Color color) {
    auto label = std::make_shared<we::runtime::kindui::Label>(text, color, size);
    label->SetHorizontalAlignment(we::runtime::kindui::HorizontalAlignment::Left);
    return label;
}

inline std::shared_ptr<we::runtime::kindui::Column> MakePageBodyPadding() {
    auto content = std::make_shared<we::runtime::kindui::Column>();
    content->Padding(we::runtime::kindui::Margin{
        kLauncherContentPadX * LScale(),
        LMetric(we::runtime::kindui::MetricToken::Space4) * LScale(),
        kLauncherContentPadX * LScale(),
        LMetric(we::runtime::kindui::MetricToken::Space4) * LScale()
    });
    content->Gap(LMetric(we::runtime::kindui::MetricToken::Space4) * LScale());
    content->SetHorizontalAlignment(we::runtime::kindui::HorizontalAlignment::Fill);
    return content;
}

inline std::shared_ptr<we::runtime::kindui::Widget> MakeSectionHeader(
    const std::string& title,
    const std::string& subtitle = {}) {
    auto heading = std::make_shared<we::runtime::kindui::SectionHeader>(title, subtitle);
    heading->SetHorizontalAlignment(we::runtime::kindui::HorizontalAlignment::Fill);
    return heading;
}

inline std::string JoinList(const std::vector<std::string>& items, const char* sep = ", ") {
    if (items.empty()) {
        return "—";
    }
    std::string out;
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i > 0) {
            out += sep;
        }
        out += items[i];
    }
    return out;
}

inline const ProjectTemplateInfo* FindTemplateById(
    const std::vector<ProjectTemplateInfo>& templates,
    const std::string& id) {
    for (const auto& tmpl : templates) {
        if (tmpl.id == id) {
            return &tmpl;
        }
    }
    return templates.empty() ? nullptr : &templates.front();
}

} // namespace we::programs::welauncher
