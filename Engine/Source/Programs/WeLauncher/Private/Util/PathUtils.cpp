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

namespace we::programs::welauncher {

std::filesystem::path PathUtils::GetExecutableDirectory() {
    return we::core::GetExecutableDirectory();
}

std::optional<std::filesystem::path> PathUtils::FindEngineRoot(const std::filesystem::path& start) {
    auto tryFrom = [](const std::filesystem::path& dir) -> std::optional<std::filesystem::path> {
        if (dir.empty()) {
            return std::nullopt;
        }
        std::error_code ec;
        std::filesystem::path current = std::filesystem::weakly_canonical(dir, ec);
        if (ec) {
            current = dir;
        }
        while (!current.empty()) {
            const auto descriptor = current / "WindEffects.engine";
            if (std::filesystem::exists(descriptor)) {
                return current;
            }
            const auto parent = current.parent_path();
            if (parent == current) {
                break;
            }
            current = parent;
        }
        return std::nullopt;
    };

    if (const char* envRoot = std::getenv("WE_ENGINE_ROOT")) {
        if (auto found = tryFrom(FromUtf8(envRoot))) {
            return found;
        }
    }
    if (const char* envRoot = std::getenv("WE_PROJECT_ROOT")) {
        if (auto found = tryFrom(FromUtf8(envRoot))) {
            return found;
        }
    }

    if (auto found = tryFrom(start)) {
        return found;
    }
    return tryFrom(std::filesystem::current_path());
}

std::filesystem::path PathUtils::ResolveRelative(const std::filesystem::path& root, const std::string& relative) {
    std::string normalized = relative;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    std::error_code ec;
    const auto combined = root / FromUtf8(normalized);
    auto canonical = std::filesystem::weakly_canonical(combined, ec);
    if (ec) {
        return combined;
    }
    return canonical;
}

std::string PathUtils::ToUtf8(const std::filesystem::path& path) {
#if defined(_WIN32)
    const std::wstring wide = path.native();
    if (wide.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        nullptr,
        0,
        nullptr,
        nullptr);
    if (size <= 0) {
        return {};
    }
    std::string out(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        out.data(),
        size,
        nullptr,
        nullptr);
    return out;
#else
    return path.string();
#endif
}

std::filesystem::path PathUtils::FromUtf8(const std::string& path) {
#if defined(_WIN32)
    if (path.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(
        CP_UTF8,
        0,
        path.data(),
        static_cast<int>(path.size()),
        nullptr,
        0);
    if (size <= 0) {
        return std::filesystem::path(path);
    }
    std::wstring wide(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        0,
        path.data(),
        static_cast<int>(path.size()),
        wide.data(),
        size);
    return std::filesystem::path(wide);
#else
    return std::filesystem::u8path(path);
#endif
}

std::filesystem::path PathUtils::GetDefaultProjectsRoot() {
#if defined(_WIN32)
    // Prefer a stable ASCII path under the user profile to avoid OneDrive / locale
    // path encoding issues that previously corrupted launcher.json and crashed init.
    const char* profile = std::getenv("USERPROFILE");
    if (profile && profile[0] != '\0') {
        return std::filesystem::path(profile) / "WindEffects" / "Projects";
    }
    wchar_t* docs = nullptr;
    std::filesystem::path base;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs)) && docs) {
        base = docs;
        CoTaskMemFree(docs);
        return base / "WindEffects" / "Projects";
    }
    return std::filesystem::path(".") / "WindEffects" / "Projects";
#else
    const char* home = std::getenv("HOME");
    return std::filesystem::path(home ? home : ".") / "WindEffects" / "Projects";
#endif
}

std::filesystem::path PathUtils::GetLauncherDataRoot() {
#if defined(_WIN32)
    wchar_t* localApp = nullptr;
    std::filesystem::path base;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localApp)) && localApp) {
        base = localApp;
        CoTaskMemFree(localApp);
    } else {
        base = std::filesystem::path(std::getenv("LOCALAPPDATA") ? std::getenv("LOCALAPPDATA") : ".");
    }
    return base / "WindEffects";
#else
    const char* config = std::getenv("XDG_CONFIG_HOME");
    if (!config) {
        const char* home = std::getenv("HOME");
        return std::filesystem::path(home ? home : ".") / ".config" / "windeffects";
    }
    return std::filesystem::path(config) / "windeffects";
#endif
}

std::filesystem::path PathUtils::GetLauncherSettingsPath() {
    return GetLauncherDataRoot() / "launcher.json";
}

std::filesystem::path PathUtils::GetLauncherCacheRoot() {
    return GetLauncherDataRoot() / "Cache";
}

std::filesystem::path PathUtils::GetThumbnailCacheRoot() {
    return GetLauncherDataRoot() / "Thumbnails";
}

std::filesystem::path PathUtils::GetLauncherLogsRoot() {
    return GetLauncherDataRoot() / "Logs";
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
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &time);
#else
    gmtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string PathUtils::SanitizeProjectName(const std::string& name) {
    std::string out;
    out.reserve(name.size());
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') {
            out.push_back(c);
        } else if (std::isspace(static_cast<unsigned char>(c))) {
            out.push_back('_');
        }
    }
    if (out.empty()) {
        out = "Project";
    }
    return out;
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
