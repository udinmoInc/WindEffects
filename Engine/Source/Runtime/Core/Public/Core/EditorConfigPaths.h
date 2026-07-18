#pragma once

#include "Core/Paths.h"

#include <filesystem>

namespace we::core {

inline std::filesystem::path GetEngineConfigDirectory() {
    return PathService::Get().EngineConfigRoot();
}

inline std::filesystem::path GetEditorConfigDirectory() {
    return GetEngineConfigDirectory() / layout::kEditor;
}

inline std::filesystem::path GetEditorConfigPath(const char* fileName) {
    return GetEditorConfigDirectory() / fileName;
}

inline void MigrateLegacyEditorConfigFile(const std::filesystem::path& configPath) {
    const auto fileName = configPath.filename();
    const auto& paths = PathService::Get();
    const std::filesystem::path exe = paths.ExecutableDirectory();

    const std::filesystem::path legacyCandidates[] = {
        exe / layout::kConfig / layout::kEditor / fileName,
        exe / "Settings" / layout::kEditor / fileName,
        exe / layout::kSaved / layout::kConfig / fileName,
        paths.SavedRoot() / layout::kConfig / fileName,
    };

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

    for (const auto& legacyPath : legacyCandidates) {
        migrateFrom(legacyPath);
    }

    if (std::filesystem::exists(configPath, ec)) {
        for (const auto& legacyPath : legacyCandidates) {
            if (std::filesystem::exists(legacyPath, ec)) {
                std::filesystem::remove(legacyPath, ec);
            }
        }
    }
}

inline std::filesystem::path ResolveEditorConfigPath(const char* fileName) {
    const auto path = GetEditorConfigPath(fileName);
    MigrateLegacyEditorConfigFile(path);
    return path;
}

} // namespace we::core
