#pragma once

#include "Projects/Export.h"

#include <filesystem>
#include <optional>
#include <string>

namespace we::projects {

/// Engine-owned install context. Never holds project assets or game content.
/// Disk locations are published to we::core::PathService on Initialize.
class PROJECTS_API EngineContext {
public:
    static EngineContext& Get();

    void Initialize(const std::filesystem::path& executableDirectory);
    void Shutdown();

    [[nodiscard]] bool IsInitialized() const { return m_Initialized; }
    [[nodiscard]] const std::filesystem::path& EngineRoot() const { return m_EngineRoot; }
    [[nodiscard]] const std::filesystem::path& EngineContentRoot() const { return m_EngineContentRoot; }
    [[nodiscard]] const std::filesystem::path& EngineConfigRoot() const { return m_EngineConfigRoot; }
    [[nodiscard]] const std::filesystem::path& EngineShadersRoot() const { return m_EngineShadersRoot; }
    [[nodiscard]] const std::filesystem::path& EngineBinariesRoot() const { return m_EngineBinariesRoot; }
    [[nodiscard]] const std::filesystem::path& TemplatesRoot() const { return m_TemplatesRoot; }
    [[nodiscard]] const std::filesystem::path& ExecutableDirectory() const { return m_ExecutableDirectory; }
    [[nodiscard]] const std::string& EngineVersion() const { return m_EngineVersion; }

    /// Resolve engine root by walking parents for WindEffects.engine (via PathService).
    [[nodiscard]] static std::optional<std::filesystem::path> FindEngineRoot(
        const std::filesystem::path& startDirectory);

private:
    EngineContext() = default;

    bool m_Initialized = false;
    std::filesystem::path m_ExecutableDirectory;
    std::filesystem::path m_EngineRoot;
    std::filesystem::path m_EngineContentRoot;
    std::filesystem::path m_EngineConfigRoot;
    std::filesystem::path m_EngineShadersRoot;
    std::filesystem::path m_EngineBinariesRoot;
    std::filesystem::path m_TemplatesRoot;
    std::string m_EngineVersion = "0.0.0";
};

} // namespace we::projects
