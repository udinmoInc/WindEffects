#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace we::programs::welauncher {

class PathUtils {
public:
    static std::filesystem::path GetExecutableDirectory();
    static std::optional<std::filesystem::path> FindEngineRoot(const std::filesystem::path& start);
    static std::filesystem::path ResolveRelative(const std::filesystem::path& root, const std::string& relative);
    static std::string ToUtf8(const std::filesystem::path& path);
    static std::filesystem::path FromUtf8(const std::string& path);
    static std::filesystem::path GetDefaultProjectsRoot();
    static std::filesystem::path GetLauncherSettingsPath();
    static std::string GetUtcNowIso8601();
    static std::string SanitizeProjectName(const std::string& name);
    static bool IsPathInsideEngineInstall(const std::filesystem::path& path, const std::filesystem::path& engineRoot);
    static std::string ReplaceTokens(std::string text, const std::string& projectName);
};

} // namespace we::programs::welauncher
