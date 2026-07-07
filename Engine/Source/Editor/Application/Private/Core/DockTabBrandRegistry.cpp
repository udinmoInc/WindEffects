#include "Core/DockTabBrandRegistry.h"

namespace we::UI {

DockTabBrandRegistry& DockTabBrandRegistry::Get() {
    static DockTabBrandRegistry instance;
    return instance;
}

void DockTabBrandRegistry::RegisterBrand(const std::string& panelTitle, VkDescriptorSet texture, float logicalSize) {
    m_Brands[panelTitle] = DockTabBrand{ texture, logicalSize };
}

bool DockTabBrandRegistry::HasBrand(const std::string& panelTitle) const {
    return m_Brands.find(panelTitle) != m_Brands.end();
}

DockTabBrand DockTabBrandRegistry::GetBrand(const std::string& panelTitle) const {
    auto it = m_Brands.find(panelTitle);
    if (it != m_Brands.end()) {
        return it->second;
    }
    return {};
}

} // namespace we::UI
