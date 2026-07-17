#pragma once

#include "WindEffects/Editor/UI/Docking/IDockManager.h"

#include <cctype>
#include <string>
#include <string_view>

namespace we::editor::panels {
using ::we::editor::docking::DockZone;
using ::we::editor::docking::DockPanelDescriptor;

inline std::string FormatPanelTitle(std::string_view panelId) {
    std::string title;
    title.reserve(panelId.size() + 4);
    for (size_t i = 0; i < panelId.size(); ++i) {
        const char c = panelId[i];
        if (i > 0 && std::isupper(static_cast<unsigned char>(c))
            && std::islower(static_cast<unsigned char>(panelId[i - 1]))) {
            title.push_back(' ');
        }
        title.push_back(c);
    }
    return title;
}

class PanelDescriptorBuilder {
public:
    explicit PanelDescriptorBuilder(std::string_view panelId) {
        m_Descriptor.id = std::string(panelId);
        m_Descriptor.title = FormatPanelTitle(panelId);
    }

    PanelDescriptorBuilder& Title(std::string_view title) {
        m_Descriptor.title = std::string(title);
        return *this;
    }

    PanelDescriptorBuilder& Icon(std::string_view iconResource) {
        m_Descriptor.iconResource = std::string(iconResource);
        return *this;
    }

    PanelDescriptorBuilder& Zone(DockZone zone) {
        m_Descriptor.defaultZone = zone;
        return *this;
    }

    PanelDescriptorBuilder& Visible(bool visible = true) {
        m_Descriptor.defaultVisible = visible;
        return *this;
    }

    PanelDescriptorBuilder& Hidden() {
        m_Descriptor.defaultVisible = false;
        return *this;
    }

    PanelDescriptorBuilder& SortOrder(int order) {
        m_Descriptor.sortOrder = order;
        return *this;
    }

    PanelDescriptorBuilder& WindowMenu(std::string_view label = {}) {
        m_Descriptor.showInWindowMenu = true;
        m_Descriptor.windowMenuLabel = label.empty()
            ? m_Descriptor.title
            : std::string(label);
        return *this;
    }

    PanelDescriptorBuilder& NoWindowMenu() {
        m_Descriptor.showInWindowMenu = false;
        m_Descriptor.windowMenuLabel.clear();
        return *this;
    }

    [[nodiscard]] DockPanelDescriptor Build() const { return m_Descriptor; }
    operator DockPanelDescriptor() const { return m_Descriptor; }

private:
    DockPanelDescriptor m_Descriptor;
};

#define WE_PANEL(PanelId) ::we::editor::panels::PanelDescriptorBuilder(#PanelId)

} // namespace we::editor::panels
