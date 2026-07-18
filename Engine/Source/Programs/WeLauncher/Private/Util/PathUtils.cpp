#include "Util/PathUtils.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>
#include "Platform/UndefWin32Macros.h"
#endif

#include "Core/BuildPaths.h"
#include "Core/Paths.h"
#include "Projects/ProjectLifecycle.h"

namespace we::programs::welauncher {

std::filesystem::path PathUtils::GetExecutableDirectory() {
    return we::core::PathService::Get().ExecutableDirectory();
}

std::optional<std::filesystem::path> PathUtils::FindEngineRoot(const std::filesystem::path& start) {
    return we::core::PathService::FindEngineRoot(start);
}

std::filesystem::path PathUtils::ResolveRelative(const std::filesystem::path& root, const std::string& relative) {
    return we::core::PathService::ResolveRelative(root, we::core::PathService::FromUtf8(relative));
}

std::string PathUtils::ToUtf8(const std::filesystem::path& path) {
    return we::core::PathService::ToUtf8(path);
}

std::filesystem::path PathUtils::FromUtf8(const std::string& path) {
    return we::core::PathService::FromUtf8(path);
}

std::filesystem::path PathUtils::GetDefaultProjectsRoot() {
    return we::core::PathService::Get().DefaultProjectsRoot();
}

std::filesystem::path PathUtils::GetLauncherDataRoot() {
    return we::core::PathService::Get().UserDataRoot();
}

std::filesystem::path PathUtils::GetLauncherSettingsPath() {
    return GetLauncherDataRoot() / "launcher.json";
}

std::filesystem::path PathUtils::GetLauncherCacheRoot() {
    return we::core::PathService::Get().UserCacheRoot();
}

std::filesystem::path PathUtils::GetThumbnailCacheRoot() {
    return GetLauncherDataRoot() / we::core::layout::kThumbnails;
}

std::filesystem::path PathUtils::GetLauncherLogsRoot() {
    return we::core::PathService::Get().UserLogsRoot();
}

std::uint64_t PathUtils::EstimateDirectoryBytes(const std::filesystem::path& root) {
    std::error_code ec;
    if (!std::filesystem::exists(root, ec) || !std::filesystem::is_directory(root, ec)) {
        return 0;
    }
    std::uint64_t total = 0;
    for (std::filesystem::recursive_directory_iterator it(
             root,
             std::filesystem::directory_options::skip_permission_denied,
             ec),
         end;
         it != end;
         it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        if (!it->is_regular_file(ec)) {
            continue;
        }
        total += static_cast<std::uint64_t>(it->file_size(ec));
        if (ec) {
            ec.clear();
        }
    }
    return total;
}

std::string PathUtils::FormatByteSize(std::uint64_t bytes) {
    constexpr double kKb = 1024.0;
    constexpr double kMb = kKb * 1024.0;
    constexpr double kGb = kMb * 1024.0;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    if (bytes >= static_cast<std::uint64_t>(kGb)) {
        oss << (static_cast<double>(bytes) / kGb) << " GB";
    } else if (bytes >= static_cast<std::uint64_t>(kMb)) {
        oss << (static_cast<double>(bytes) / kMb) << " MB";
    } else if (bytes >= static_cast<std::uint64_t>(kKb)) {
        oss << (static_cast<double>(bytes) / kKb) << " KB";
    } else {
        oss << std::setprecision(0) << bytes << " B";
    }
    return oss.str();
}

bool PathUtils::ClearDirectoryContents(const std::filesystem::path& root) {
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    if (ec) {
        return false;
    }
    for (std::filesystem::directory_iterator it(root, ec), end; it != end; it.increment(ec)) {
        if (ec) {
            return false;
        }
        std::filesystem::remove_all(it->path(), ec);
        if (ec) {
            return false;
        }
    }
    return true;
}

std::string PathUtils::GetUtcNowIso8601() {
    return we::projects::ProjectLifecycle::NowUtc();
}

std::string PathUtils::SanitizeProjectName(const std::string& name) {
    return we::projects::ProjectLifecycle::SanitizeProjectName(name);
}

bool PathUtils::IsPathInsideEngineInstall(const std::filesystem::path& path, const std::filesystem::path& engineRoot) {
    std::error_code ec;
    const auto canonicalPath = std::filesystem::weakly_canonical(path, ec);
    const auto canonicalEngine = std::filesystem::weakly_canonical(engineRoot, ec);
    if (ec) {
        return false;
    }
    auto rel = std::filesystem::relative(canonicalPath, canonicalEngine, ec);
    if (ec || rel.empty()) {
        return canonicalPath == canonicalEngine;
    }
    const std::string relStr = ToUtf8(rel);
    return relStr.rfind("..", 0) != 0;
}

std::string PathUtils::ReplaceTokens(std::string text, const std::string& projectName) {
    const auto replaceAll = [&](const std::string& token, const std::string& value) {
        std::size_t pos = 0;
        while ((pos = text.find(token, pos)) != std::string::npos) {
            text.replace(pos, token.size(), value);
            pos += value.size();
        }
    };
    replaceAll("{{ProjectName}}", projectName);
    replaceAll("{{PROJECT_NAME}}", projectName);
    return text;
}

} // namespace we::programs::welauncher
