#pragma once

#include "Model/WeProjectDescriptor.h"

#include <filesystem>
#include <vector>

namespace we::programs::welauncher {

struct EngineDescriptorData {
    std::filesystem::path engineRoot;
    std::string schema;
    std::string engineVersion;
    std::filesystem::path programsRoot;
    std::filesystem::path buildRoot;
    std::filesystem::path assetsRoot;
    std::filesystem::path projectsRoot;
    std::filesystem::path templatesRoot;
};

class EngineDiscoveryService {
public:
    bool Discover(const std::filesystem::path& startDirectory);

    [[nodiscard]] const EngineDescriptorData& Current() const { return m_Current; }
    [[nodiscard]] const std::vector<EngineInstallInfo>& InstalledEngines() const { return m_Installed; }
    [[nodiscard]] bool HasEngine() const { return !m_Current.engineRoot.empty(); }

    [[nodiscard]] std::filesystem::path ResolveEditorExecutable(const std::string& buildConfig) const;

private:
    bool TryLoadDescriptor(const std::filesystem::path& engineRoot, EngineDescriptorData& out);

    EngineDescriptorData m_Current{};
    std::vector<EngineInstallInfo> m_Installed;
};

} // namespace we::programs::welauncher
