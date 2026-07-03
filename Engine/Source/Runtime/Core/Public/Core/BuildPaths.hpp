#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
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

inline const std::unordered_map<std::string, const char*>& GetModuleCategoryMap() {
    static const std::unordered_map<std::string, const char*> kCategories = {
        {"Application", "Programs"},
        {"CrashReporter", "Programs"},
        {"Launcher", "Programs"},
        {"MainFrame", "Editor"},
        {"Docking", "Editor"},
        {"Viewport", "Editor"},
        {"ContentBrowser", "Editor"},
        {"WorldOutliner", "Editor"},
        {"PropertyEditor", "Editor"},
        {"Details", "Editor"},
        {"Toolbar", "Editor"},
        {"Menus", "Editor"},
        {"ToolsPanel", "Editor"},
        {"PlaceActors", "Editor"},
        {"Environment", "Editor"},
        {"EditorGridRenderer", "Editor"},
        {"Core", ""},
        {"CoreUObject", ""},
        {"Engine", ""},
    };
    return kCategories;
}

inline std::string GetModuleBinaryFileName(std::string_view moduleName) {
    return "WE" + std::string(moduleName) + ".dll";
}

inline std::optional<std::filesystem::path> ResolveModuleLibraryPath(std::string_view logicalName) {
    const std::string moduleName = StripLegacyModulePrefix(logicalName);
    const auto root = GetConfigurationRoot();
    if (root.empty()) {
        return std::nullopt;
    }

    const std::string binaryName = GetModuleBinaryFileName(moduleName);

    const auto& categories = GetModuleCategoryMap();
    if (const auto categoryIt = categories.find(moduleName); categoryIt != categories.end()) {
        const char* category = categoryIt->second;
        if (category == nullptr || category[0] == '\0') {
            const auto bootstrapPath = root / binaryName;
            return std::filesystem::exists(bootstrapPath) ? std::optional(bootstrapPath) : std::nullopt;
        }

        const auto modulePath = root / category / moduleName / binaryName;
        return std::filesystem::exists(modulePath) ? std::optional(modulePath) : std::nullopt;
    }

    const char* searchCategories[] = {
        "Runtime",
        "Editor",
        "Programs",
        "Developer",
        "Plugins",
        "ThirdParty",
    };

    for (const char* category : searchCategories) {
        const auto modulePath = root / category / moduleName / binaryName;
        if (std::filesystem::exists(modulePath)) {
            return modulePath;
        }
    }

    const auto bootstrapPath = root / binaryName;
    return std::filesystem::exists(bootstrapPath) ? std::optional(bootstrapPath) : std::nullopt;
}

inline std::optional<std::filesystem::path> ResolveDelayLoadLibraryPath(std::string_view dllFileName) {
    std::string_view fileName = dllFileName;
    if (fileName.ends_with(".dll")) {
        fileName.remove_suffix(4);
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

    const auto directPath = root / dllFileName;
    if (std::filesystem::exists(directPath)) {
        return directPath;
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

    const char* categories[] = {
        "Runtime",
        "Editor",
        "Programs",
        "Developer",
        "Plugins",
        "ThirdParty",
    };

    for (const char* category : categories) {
        const auto categoryPath = root / category;
        if (!std::filesystem::exists(categoryPath)) {
            continue;
        }

        for (const auto& entry : std::filesystem::directory_iterator(categoryPath)) {
            if (!entry.is_directory()) {
                continue;
            }

            AddDllDirectory(entry.path().wstring().c_str());
        }
    }
#endif
}

} // namespace we::core
