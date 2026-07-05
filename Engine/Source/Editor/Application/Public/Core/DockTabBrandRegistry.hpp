#pragma once

#include "Application/Export.hpp"

#include <string>
#include <unordered_map>
#include <volk.h>

namespace we::UI {

struct DockTabBrand {
    VkDescriptorSet texture = VK_NULL_HANDLE;
    float logicalSize = 16.0f;
};

/// Maps panel titles to high-DPI rasterized SVG brand marks for dock tab chrome.
class APPLICATION_API DockTabBrandRegistry {
public:
    static DockTabBrandRegistry& Get();

    void RegisterBrand(const std::string& panelTitle, VkDescriptorSet texture, float logicalSize);
    bool HasBrand(const std::string& panelTitle) const;
    DockTabBrand GetBrand(const std::string& panelTitle) const;

private:
    DockTabBrandRegistry() = default;

    std::unordered_map<std::string, DockTabBrand> m_Brands;
};

} // namespace we::UI
