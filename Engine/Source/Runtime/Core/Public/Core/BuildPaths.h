#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace we::core {

inline std::filesystem::path GetExecutableDirectory() {
#if defined(_WIN32)
    wchar_t exePath[MAX_PATH]{};
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0) {
        return {};
    }
    return std::filesystem::path(exePath).parent_path();
#else
    return std::filesystem::current_path();
#endif
}

inline std::filesystem::path GetConfigurationRoot() {
    return GetExecutableDirectory();
}

inline std::string StripLegacyModulePrefix(std::string_view logicalName) {
    constexpr std::string_view kPrefix = "WindEffects-";
    if (logicalName.starts_with(kPrefix)) {
        return std::string(logicalName.substr(kPrefix.size()));
    }
    return std::string(logicalName);
}

inline std::vector<std::string> BuildModuleBinaryCandidates(std::string_view moduleName) {
    const std::string name(StripLegacyModulePrefix(moduleName));
    return {
        "Windeffects" + name + ".dll",
        "WE" + name + ".dll",
    };
}

inline std::optional<std::filesystem::path> FindExistingBinary(
    const std::filesystem::path& directory,
    const std::vector<std::string>& candidates) {
    if (directory.empty() || !std::filesystem::exists(directory)) {
        return std::nullopt;
    }

    for (const auto& candidate : candidates) {
        const auto path = directory / candidate;
        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    return std::nullopt;
}

inline std::optional<std::filesystem::path> ResolveModuleLibraryPath(std::string_view logicalName) {
    const auto root = GetConfigurationRoot();
    if (root.empty()) {
        return std::nullopt;
    }

    const auto candidates = BuildModuleBinaryCandidates(logicalName);

    if (const auto bootstrapPath = FindExistingBinary(root, candidates)) {
        return bootstrapPath;
    }

    if (const auto enginePath = FindExistingBinary(root / "Engine" / "Binaries", candidates)) {
        return enginePath;
    }

    const auto pluginsRoot = root / "Plugins";
    if (std::filesystem::exists(pluginsRoot)) {
        for (const auto& entry : std::filesystem::directory_iterator(pluginsRoot)) {
            if (!entry.is_directory()) {
                continue;
            }

            if (const auto pluginPath = FindExistingBinary(entry.path(), candidates)) {
                return pluginPath;
            }
        }
    }

    if (const auto thirdPartyPath = FindExistingBinary(root / "ThirdParty", candidates)) {
        return thirdPartyPath;
    }

    return std::nullopt;
}

inline std::optional<std::filesystem::path> ResolveDelayLoadLibraryPath(std::string_view dllFileName) {
    std::string_view fileName = dllFileName;
    if (fileName.ends_with(".dll")) {
        fileName.remove_suffix(4);
    }

    if (fileName.starts_with("Windeffects")) {
        fileName.remove_prefix(std::string_view("Windeffects").size());
        return ResolveModuleLibraryPath(fileName);
    }

    if (fileName.starts_with("WE")) {
        fileName.remove_prefix(2);
        return ResolveModuleLibraryPath(fileName);
    }

    if (fileName.starts_with("WindEffects-")) {
        return ResolveModuleLibraryPath(fileName);
    }

    const auto root = GetConfigurationRoot();
    if (root.empty()) {
        return std::nullopt;
    }

    const std::string dllName(dllFileName);
    const std::array searchRoots = {
        root / "ThirdParty",
        root / "Engine" / "Binaries",
        root
    };

    for (const auto& searchRoot : searchRoots) {
        if (!std::filesystem::exists(searchRoot)) {
            continue;
        }

        const auto directPath = searchRoot / dllName;
        if (std::filesystem::exists(directPath)) {
            return directPath;
        }
    }

    return std::nullopt;
}

inline void ConfigureModuleSearchPaths() {
#if defined(_WIN32)
    const auto root = GetConfigurationRoot();
    if (root.empty()) {
        return;
    }

    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_USER_DIRS);

    AddDllDirectory(root.wstring().c_str());

    const std::array searchRoots = {
        root / "Engine" / "Binaries",
        root / "ThirdParty",
        root / "Plugins"
    };

    for (const auto& searchRoot : searchRoots) {
        if (!std::filesystem::exists(searchRoot)) {
            continue;
        }

        if (searchRoot.filename() == "Plugins") {
            for (const auto& entry : std::filesystem::directory_iterator(searchRoot)) {
                if (entry.is_directory()) {
                    AddDllDirectory(entry.path().wstring().c_str());
                }
            }
            continue;
        }

        AddDllDirectory(searchRoot.wstring().c_str());
    }
#endif
}

} // namespace we::core
