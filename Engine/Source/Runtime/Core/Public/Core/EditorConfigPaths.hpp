#pragma once

#include <filesystem>

namespace we::core {

inline std::filesystem::path GetEditorConfigDirectory() {
    return std::filesystem::path("Settings") / "Editor";
}

inline std::filesystem::path GetEditorConfigPath(const char* fileName) {
    return GetEditorConfigDirectory() / fileName;
}

inline void MigrateLegacyEditorConfigFile(const std::filesystem::path& configPath) {
    const auto fileName = configPath.filename();
    const std::filesystem::path legacyConfigPath = std::filesystem::path("Config") / "Editor" / fileName;
    const std::filesystem::path legacySavedPath = std::filesystem::path("Saved") / "Config" / fileName;
    const std::filesystem::path legacyLowerPath = std::filesystem::path("config") / "editor" / fileName;

    std::error_code ec;
    std::filesystem::create_directories(configPath.parent_path(), ec);

    const auto migrateFrom = [&](const std::filesystem::path& legacyPath) {
        if (!std::filesystem::exists(legacyPath, ec)) {
            return;
        }

        if (!std::filesystem::exists(configPath, ec)) {
            std::filesystem::copy_file(
                legacyPath,
                configPath,
                std::filesystem::copy_options::overwrite_existing,
                ec);
            return;
        }

        const auto legacyTime = std::filesystem::last_write_time(legacyPath, ec);
        const auto configTime = std::filesystem::last_write_time(configPath, ec);
        if (!ec && legacyTime > configTime) {
            std::filesystem::copy_file(
                legacyPath,
                configPath,
                std::filesystem::copy_options::overwrite_existing,
                ec);
        }
    };

    migrateFrom(legacyConfigPath);
    migrateFrom(legacySavedPath);
    migrateFrom(legacyLowerPath);

    if (std::filesystem::exists(configPath, ec)) {
        if (std::filesystem::exists(legacyConfigPath, ec)) {
            std::filesystem::remove(legacyConfigPath, ec);
        }
        if (std::filesystem::exists(legacySavedPath, ec)) {
            std::filesystem::remove(legacySavedPath, ec);
        }
        if (std::filesystem::exists(legacyLowerPath, ec)) {
            std::filesystem::remove(legacyLowerPath, ec);
        }
    }
}

inline std::filesystem::path ResolveEditorConfigPath(const char* fileName) {
    const auto path = GetEditorConfigPath(fileName);
    MigrateLegacyEditorConfigFile(path);
    return path;
}

} // namespace we::core
