#include "Services/EditorLaunchService.h"

#include "Platform/PlatformSDK.h"
#include "Util/PathUtils.h"

namespace we::programs::welauncher {

EditorLaunchService::EditorLaunchService(
    EngineDiscoveryService& engines,
    LauncherSettingsStore& settings,
    ProjectService& projects)
    : m_Engines(engines)
    , m_Settings(settings)
    , m_Projects(projects) {
}

EditorLaunchResult EditorLaunchService::Launch(const std::filesystem::path& weprojPath) {
    EditorLaunchResult result{};
    const auto openResult = m_Projects.OpenProject(weprojPath);
    if (!openResult.success) {
        result.message = openResult.message;
        return result;
    }

    const auto editorExe = m_Engines.ResolveEditorExecutable(m_Settings.Settings().lastBuildConfig);
    if (editorExe.empty() || !std::filesystem::exists(editorExe)) {
        result.message = "Editor executable not found. Build the Editor target first.";
        return result;
    }

    auto& platform = we::platform::Platform::Get();
    const std::string exeUtf8 = PathUtils::ToUtf8(editorExe);
    const std::string projectArg = PathUtils::ToUtf8(weprojPath);
    const std::string workDir = PathUtils::ToUtf8(weprojPath.parent_path());

    const auto launchResult = platform.LaunchProcess({
        .executable = exeUtf8.c_str(),
        .arguments = { "-project", projectArg },
        .workingDirectory = workDir.c_str(),
        .waitForExit = false,
        .detach = true,
    });

    if (!launchResult.Ok()) {
        result.message = launchResult.error.message.empty() ? "Failed to launch editor." : launchResult.error.message;
        return result;
    }

    result.success = true;
    result.message = "Editor launched.";
    return result;
}

} // namespace we::programs::welauncher
