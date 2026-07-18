#include "Projects/ProjectContext.h"

#include "Projects/EngineContext.h"
#include "JsonIO.h"
#include "Projects/ProjectLifecycle.h"
#include "Projects/RecentProjectsStore.h"

#include "Core/Logger.h"

namespace we::projects {

ProjectContext& ProjectContext::Get() {
    static ProjectContext instance;
    return instance;
}

ProjectValidationResult ProjectContext::Load(const std::filesystem::path& weprojPath) {
    if (m_Loaded) {
        Unload();
    }

    auto validation = ProjectLifecycle::ValidateProjectPath(
        weprojPath,
        EngineContext::Get().EngineVersion());
    if (!validation.ok && !validation.needsUpgrade) {
        return validation;
    }

    auto descriptor = ProjectLifecycle::ReadDescriptor(weprojPath);
    if (!descriptor) {
        validation.ok = false;
        validation.message = "Failed to read project descriptor.";
        return validation;
    }

    m_WeprojPath = std::filesystem::absolute(weprojPath).lexically_normal();
    m_ProjectRoot = m_WeprojPath.parent_path();
    m_Descriptor = std::move(*descriptor);

    const std::string contentRel = m_Descriptor.contentDirectory.empty()
        ? "Content"
        : m_Descriptor.contentDirectory;
    m_ContentRoot = m_ProjectRoot / contentRel;
    m_PluginsRoot = m_ProjectRoot / "Plugins";
    m_ConfigRoot = m_ProjectRoot / "Config";
    m_SavedRoot = m_ProjectRoot / "Saved";
    m_IntermediateRoot = m_ProjectRoot / "Intermediate";
    m_CurrentMap = m_Descriptor.startupMap;
    m_Loaded = true;

    m_Descriptor.lastOpenedUtc = ProjectLifecycle::NowUtc();
    (void)SaveDescriptor();

    RecentProjectsStore::Get().Touch(m_WeprojPath, m_Descriptor);

    HE_INFO("[ProjectContext] Loaded project: " + m_Descriptor.displayName
        + " (" + m_WeprojPath.string() + ")");

    if (m_OnLoaded) {
        m_OnLoaded();
    }

    validation.ok = true;
    if (validation.message.empty()) {
        validation.message = "Project loaded.";
    }
    return validation;
}

bool ProjectContext::SaveDescriptor() {
    if (!m_Loaded || m_WeprojPath.empty()) {
        return false;
    }
    return ProjectLifecycle::WriteDescriptor(m_WeprojPath, m_Descriptor);
}

void ProjectContext::Unload() {
    if (!m_Loaded) {
        return;
    }

    HE_INFO("[ProjectContext] Unloading project: " + m_Descriptor.displayName);

    if (m_OnUnloaded) {
        m_OnUnloaded();
    }

    m_Loaded = false;
    m_Descriptor = {};
    m_WeprojPath.clear();
    m_ProjectRoot.clear();
    m_ContentRoot.clear();
    m_PluginsRoot.clear();
    m_ConfigRoot.clear();
    m_SavedRoot.clear();
    m_IntermediateRoot.clear();
    m_CurrentMap.clear();
}

} // namespace we::projects
