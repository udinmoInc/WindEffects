#include "WindEffects/Editor/UI/Resources/ModuleResourceRegistry.h"

namespace WindEffects::Editor::UI {

void ModuleResourceRegistry::RegisterDescriptor(const ResourceDescriptor& descriptor) {
    std::lock_guard lock(m_Mutex);
    m_Resources[descriptor.id.ToString()] = descriptor;
}

void ModuleResourceRegistry::RegisterBundle(std::shared_ptr<IModuleResourceBundle> bundle) {
    if (!bundle) return;

    std::lock_guard lock(m_Mutex);
    const std::string moduleId(bundle->GetModuleId());
    m_Bundles[moduleId] = bundle;
    bundle->RegisterResources(*this);
}

void ModuleResourceRegistry::UnregisterBundle(std::string_view moduleId) {
    std::lock_guard lock(m_Mutex);
    m_Bundles.erase(std::string(moduleId));

    for (auto it = m_Resources.begin(); it != m_Resources.end();) {
        if (it->second.id.moduleId == moduleId) {
            it = m_Resources.erase(it);
        } else {
            ++it;
        }
    }
}

bool ModuleResourceRegistry::HasResource(const ResourceId& id) const {
    std::lock_guard lock(m_Mutex);
    return m_Resources.contains(id.ToString());
}

std::string ModuleResourceRegistry::GetResourcePath(const ResourceId& id) const {
    std::lock_guard lock(m_Mutex);
    const auto it = m_Resources.find(id.ToString());
    return it != m_Resources.end() ? it->second.path : std::string{};
}

std::vector<ResourceDescriptor> ModuleResourceRegistry::GetModuleResources(std::string_view moduleId) const {
    std::lock_guard lock(m_Mutex);
    std::vector<ResourceDescriptor> result;
    for (const auto& [_, desc] : m_Resources) {
        if (desc.id.moduleId == moduleId) {
            result.push_back(desc);
        }
    }
    return result;
}

} // namespace WindEffects::Editor::UI
