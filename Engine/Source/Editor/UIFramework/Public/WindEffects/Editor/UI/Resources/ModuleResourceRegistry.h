#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Resources/IResourceRegistry.h"

#include <mutex>
#include <unordered_map>

namespace WindEffects::Editor::UI {

class ModuleResourceRegistry final : public IResourceRegistry {
public:
    void RegisterBundle(std::shared_ptr<IModuleResourceBundle> bundle) override;
    void UnregisterBundle(std::string_view moduleId) override;

    [[nodiscard]] bool HasResource(const ResourceId& id) const override;
    [[nodiscard]] std::string GetResourcePath(const ResourceId& id) const override;
    [[nodiscard]] std::vector<ResourceDescriptor> GetModuleResources(std::string_view moduleId) const override;

    void RegisterDescriptor(const ResourceDescriptor& descriptor);

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<IModuleResourceBundle>> m_Bundles;
    std::unordered_map<std::string, ResourceDescriptor> m_Resources;
};

} // namespace WindEffects::Editor::UI
