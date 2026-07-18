#pragma once

#include "Projects/Export.h"
#include "Projects/WeProjectDescriptor.h"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace we::projects {

/// Owns everything project-specific. Nothing project-related lives globally outside this.
/// Disk locations are published to we::core::PathService on Load.
class PROJECTS_API ProjectContext {
public:
    static ProjectContext& Get();

    [[nodiscard]] bool IsLoaded() const { return m_Loaded; }
    [[nodiscard]] const WeProjectDescriptor& Descriptor() const { return m_Descriptor; }
    [[nodiscard]] const std::filesystem::path& WeprojPath() const { return m_WeprojPath; }
    [[nodiscard]] const std::filesystem::path& ProjectRoot() const { return m_ProjectRoot; }
    [[nodiscard]] const std::filesystem::path& ContentRoot() const { return m_ContentRoot; }
    [[nodiscard]] const std::filesystem::path& PluginsRoot() const { return m_PluginsRoot; }
    [[nodiscard]] const std::filesystem::path& ConfigRoot() const { return m_ConfigRoot; }
    [[nodiscard]] const std::filesystem::path& SavedRoot() const { return m_SavedRoot; }
    [[nodiscard]] const std::filesystem::path& IntermediateRoot() const { return m_IntermediateRoot; }
    [[nodiscard]] const std::filesystem::path& CookedRoot() const { return m_CookedRoot; }
    [[nodiscard]] const std::filesystem::path& CacheRoot() const { return m_CacheRoot; }
    [[nodiscard]] const std::filesystem::path& LogsRoot() const { return m_LogsRoot; }
    [[nodiscard]] const std::filesystem::path& AssetPipelineRoot() const { return m_AssetPipelineRoot; }
    [[nodiscard]] const std::string& CurrentMap() const { return m_CurrentMap; }
    [[nodiscard]] const std::vector<std::string>& ModuleList() const { return m_Descriptor.modules; }
    [[nodiscard]] const std::vector<std::string>& PluginList() const { return m_Descriptor.plugins; }

    void SetCurrentMap(std::string mapPath) { m_CurrentMap = std::move(mapPath); }

    /// Load .weproj and derive roots. Does not mount content/plugins (Editor does that).
    [[nodiscard]] ProjectValidationResult Load(const std::filesystem::path& weprojPath);

    /// Persist descriptor back to disk (e.g. after upgrade / lastOpened).
    [[nodiscard]] bool SaveDescriptor();

    /// Tear down project identity. Caller must unload modules/plugins/assets first.
    void Unload();

    using ChangeCallback = std::function<void()>;
    void SetOnLoaded(ChangeCallback cb) { m_OnLoaded = std::move(cb); }
    void SetOnUnloaded(ChangeCallback cb) { m_OnUnloaded = std::move(cb); }

private:
    ProjectContext() = default;

    bool m_Loaded = false;
    WeProjectDescriptor m_Descriptor{};
    std::filesystem::path m_WeprojPath;
    std::filesystem::path m_ProjectRoot;
    std::filesystem::path m_ContentRoot;
    std::filesystem::path m_PluginsRoot;
    std::filesystem::path m_ConfigRoot;
    std::filesystem::path m_SavedRoot;
    std::filesystem::path m_IntermediateRoot;
    std::filesystem::path m_CookedRoot;
    std::filesystem::path m_CacheRoot;
    std::filesystem::path m_LogsRoot;
    std::filesystem::path m_AssetPipelineRoot;
    std::string m_CurrentMap;
    ChangeCallback m_OnLoaded;
    ChangeCallback m_OnUnloaded;
};

} // namespace we::projects
