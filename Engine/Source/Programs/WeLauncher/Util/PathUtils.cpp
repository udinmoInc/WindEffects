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
    std::filesystem::path rel = relative;
    std::string normalized = rel.generic_string();
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return std::filesystem::weakly_canonical(root / std::filesystem::path(normalized));
}

std::string PathUtils::ToUtf8(const std::filesystem::path& path) {
    return path.string();
}

std::filesystem::path PathUtils::FromUtf8(const std::string& path) {
    return std::filesystem::path(path);
}

std::filesystem::path PathUtils::GetDefaultProjectsRoot() {
#if defined(_WIN32)
    wchar_t* docs = nullptr;
    std::filesystem::path base;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs)) && docs) {
        base = docs;
        CoTaskMemFree(docs);
    } else {
        base = std::filesystem::path(std::getenv("USERPROFILE") ? std::getenv("USERPROFILE") : ".");
    }
    return base / "WindEffects" / "Projects";
#else
    const char* home = std::getenv("HOME");
    return std::filesystem::path(home ? home : ".") / "WindEffects" / "Projects";
#endif
}

std::filesystem::path PathUtils::GetLauncherSettingsPath() {
#if defined(_WIN32)
    wchar_t* localApp = nullptr;
    std::filesystem::path base;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localApp)) && localApp) {
        base = localApp;
        CoTaskMemFree(localApp);
    } else {
        base = std::filesystem::path(std::getenv("LOCALAPPDATA") ? std::getenv("LOCALAPPDATA") : ".");
    }
    return base / "WindEffects" / "launcher.json";
#else
    const char* config = std::getenv("XDG_CONFIG_HOME");
    if (!config) {
        const char* home = std::getenv("HOME");
        return std::filesystem::path(home ? home : ".") / ".config" / "windeffects" / "launcher.json";
    }
    return std::filesystem::path(config) / "windeffects" / "launcher.json";
#endif
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
    const std::string relStr = rel.generic_string();
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
