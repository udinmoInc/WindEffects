#pragma once

#include "Model/WeProjectDescriptor.h"
#include "Services/EngineDiscoveryService.h"

#include <vector>

namespace we::programs::welauncher {

class ProjectTemplateRegistry {
public:
    void Load(const EngineDescriptorData& engine);

    [[nodiscard]] const std::vector<ProjectTemplateInfo>& Templates() const { return m_Templates; }
    [[nodiscard]] const ProjectTemplateInfo* Find(std::string_view id) const;

private:
    void RegisterBuiltInFallbacks();
    void LoadFromDisk(const std::filesystem::path& templatesRoot);

    std::vector<ProjectTemplateInfo> m_Templates;
};

} // namespace we::programs::welauncher
