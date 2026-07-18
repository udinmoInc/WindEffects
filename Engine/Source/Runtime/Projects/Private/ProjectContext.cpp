#include "Projects/ProjectContext.h"

#include "Projects/EngineContext.h"
#include "JsonIO.h"
#include "Projects/ProjectLifecycle.h"
#include "Projects/RecentProjectsStore.h"

#include "Core/Logger.h"
#include "Core/Paths.h"

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

    m_WeprojPath = we::core::PathService::Absolute(weprojPath);
    m_ProjectRoot = m_WeprojPath.parent_path();
    m_Descriptor = std::move(*descriptor);

    const std::string contentRel = m_Descriptor.contentDirectory.empty()
        ? we::core::layout::kContent
        : m_Descriptor.contentDirectory;
    m_ContentRoot = we::core::PathService::ResolveRelative(m_ProjectRoot, contentRel);
    m_PluginsRoot = m_ProjectRoot / we::core::layout::kPlugins;
    m_ConfigRoot = m_ProjectRoot / we::core::layout::kConfig;
    m_SavedRoot = m_ProjectRoot / we::core::layout::kSaved;
    m_IntermediateRoot = m_ProjectRoot / we::core::layout::kIntermediate;
    m_CookedRoot = m_IntermediateRoot / we::core::layout::kCooked;
    m_CacheRoot = m_SavedRoot / we::core::layout::kCache;
    m_LogsRoot = m_SavedRoot / we::core::layout::kLogs;
    m_AssetPipelineRoot = m_IntermediateRoot / we::core::layout::kAssetPipeline;
    m_CurrentMap = m_Descriptor.startupMap;
    m_Loaded = true;

    we::core::PathService::Configuration config;
    config.projectRoot = m_ProjectRoot;
    config.contentRoot = m_ContentRoot;
    config.pluginsRoot = m_PluginsRoot;
    config.configRoot = m_ConfigRoot;
    config.savedRoot = m_SavedRoot;
    config.intermediateRoot = m_IntermediateRoot;
    config.cookedRoot = m_CookedRoot;
    config.cacheRoot = m_CacheRoot;
    config.logsRoot = m_LogsRoot;
    we::core::PathService::Get().Configure(config);

    const auto contentValidation = we::core::PathService::ValidateDirectory(m_ContentRoot);
    if (!contentValidation.ok) {
        HE_WARN("[ProjectContext] Content root: " + contentValidation.message);
    }
    const auto savedEnsure = we::core::PathService::EnsureDirectory(m_SavedRoot);
    if (!savedEnsure.ok) {
        HE_WARN("[ProjectContext] Saved root: " + savedEnsure.message);
    }

    m_Descriptor.lastOpenedUtc = ProjectLifecycle::NowUtc();
    (void)SaveDescriptor();

    RecentProjectsStore::Get().Touch(m_WeprojPath, m_Descriptor);

    HE_INFO("[ProjectContext] Loaded project: " + m_Descriptor.displayName
        + " (" + we::core::PathService::ToGeneric(m_WeprojPath) + ")");

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
    m_CookedRoot.clear();
    m_CacheRoot.clear();
    m_LogsRoot.clear();
    m_AssetPipelineRoot.clear();
    m_CurrentMap.clear();

    we::core::PathService::Get().ClearProjectRoots();
}

} // namespace we::projects
