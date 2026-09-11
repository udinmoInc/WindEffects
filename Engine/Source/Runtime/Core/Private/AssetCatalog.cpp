// ==============================================================================
// WindEffects — Core — AssetCatalog
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Core/AssetCatalog.h"
#include "Core/Logger.h"
#include "Core/Paths.h"

#include <cstdlib>
#include <fstream>
#include <mutex>

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.h>
#endif

namespace we::runtime::core {
namespace {

std::mutex g_ThemeOverrideMutex;
std::string g_ThemeOverride;

AssetCatalog BuiltInDefaults() {
    AssetCatalog catalog;
    catalog.activeTheme = "GraphiteDark";
    catalog.fonts.push_back({"Font_UI", "Roboto-Regular.wefont", true});
    catalog.shaders = {
        {"UI", "UI_VS.spv", true},
        {"AtmospherePass", "AtmospherePass_VS.spv", false},
        {"VolumetricCloudsPass", "VolumetricCloudsPass_VS.spv", false},
        {"CloudTemporalResolve", "CloudTemporalResolve_VS.spv", false},
        {"CloudCompositePass", "CloudCompositePass_VS.spv", false},
        {"FogCompositePass", "FogCompositePass_VS.spv", false},
        {"EditorGrid", "EditorGrid_VS.spv", false},
        {"SceneObject", "SceneObject_VS.spv", false},
    };
    catalog.icons.push_back({"Icon_Lucide", "icons", false});
    catalog.iconAtlasRoot = {"Icon_AtlasRoot", "Atlas", false};
    catalog.iconMeta = {"Icon_Meta", "Atlas/icons.weiconmeta", false};
    return catalog;
}

std::filesystem::path ResolveCatalogFile() {
    auto& paths = we::core::PathService::Get();
    std::vector<std::filesystem::path> candidates = {
        paths.EngineConfigRoot() / "Runtime" / "asset_catalog.json",
        paths.ConfigRoot() / "Runtime" / "asset_catalog.json",
    };
    if (const auto repo = we::core::PathService::FindRepositoryRoot(paths.ExecutableDirectory())) {
        candidates.push_back(*repo / "Engine" / "Config" / "Runtime" / "asset_catalog.json");
    }
    if (const auto found = we::core::PathService::FindExisting(candidates)) {
        return *found;
    }
    return {};
}

#if WE_HAS_NLOHMANN_JSON
AssetCatalogEntry ParseEntry(const nlohmann::json& obj, const AssetCatalogEntry& fallback) {
    AssetCatalogEntry entry = fallback;
    if (!obj.is_object()) {
        return entry;
    }
    entry.name = obj.value("name", fallback.name);
    entry.file = obj.value("file", fallback.file);
    entry.required = obj.value("required", fallback.required);
    return entry;
}
#endif

} // namespace

AssetCatalog AssetCatalogService::Load() {
    static std::mutex s_CatalogMutex;
    static AssetCatalog s_CachedCatalog;
    static std::filesystem::path s_CachedPath;
    static std::filesystem::file_time_type s_LastWriteTime{};
    static bool s_HasCache = false;

    std::lock_guard lock(s_CatalogMutex);

#if WE_HAS_NLOHMANN_JSON
    const auto path = ResolveCatalogFile();
    if (path.empty()) {
        if (!s_HasCache) {
            HE_INFO("[AssetCatalog] No asset_catalog.json found — using built-in defaults");
            s_CachedCatalog = BuiltInDefaults();
            s_HasCache = true;
        }
        return s_CachedCatalog;
    }

    std::error_code ec;
    const auto writeTime = std::filesystem::last_write_time(path, ec);
    if (s_HasCache && !ec && path == s_CachedPath && writeTime == s_LastWriteTime) {
        return s_CachedCatalog;
    }

    AssetCatalog catalog = BuiltInDefaults();
    try {
        std::ifstream input(path);
        if (!input) {
            return s_HasCache ? s_CachedCatalog : catalog;
        }
        nlohmann::json root;
        input >> root;
        catalog.schema = root.value("schema", 1);
        catalog.activeTheme = root.value("activeTheme", catalog.activeTheme);

        auto loadList = [&](const char* key, std::vector<AssetCatalogEntry>& out) {
            if (!root.contains(key) || !root[key].is_array()) {
                return;
            }
            out.clear();
            for (const auto& item : root[key]) {
                out.push_back(ParseEntry(item, {}));
            }
        };
        loadList("fonts", catalog.fonts);
        loadList("shaders", catalog.shaders);
        loadList("icons", catalog.icons);
        if (root.contains("iconAtlasRoot")) {
            catalog.iconAtlasRoot = ParseEntry(root["iconAtlasRoot"], catalog.iconAtlasRoot);
        }
        if (root.contains("iconMeta")) {
            catalog.iconMeta = ParseEntry(root["iconMeta"], catalog.iconMeta);
        }
        HE_INFO("[AssetCatalog] Loaded " + we::core::PathService::ToUtf8(path));
        s_CachedCatalog = catalog;
        s_CachedPath = path;
        s_LastWriteTime = writeTime;
        s_HasCache = true;
    } catch (const std::exception& ex) {
        HE_WARN(std::string("[AssetCatalog] Failed to parse catalog: ") + ex.what());
        if (!s_HasCache) {
            s_CachedCatalog = BuiltInDefaults();
            s_HasCache = true;
        }
    }
    return s_CachedCatalog;
#else
    if (!s_HasCache) {
        HE_INFO("[AssetCatalog] JSON unavailable — using built-in defaults");
        s_CachedCatalog = BuiltInDefaults();
        s_HasCache = true;
    }
    return s_CachedCatalog;
#endif
}

std::filesystem::path AssetCatalogService::ResolveThemeConfigPath(const AssetCatalog& catalog) {
    const std::string theme = GetActiveThemeName(catalog);
    auto& paths = we::core::PathService::Get();
    std::vector<std::filesystem::path> candidates = {
        paths.EngineConfigRoot() / "Themes" / (theme + ".json"),
        paths.ConfigRoot() / "Themes" / (theme + ".json"),
    };
    if (const auto repo = we::core::PathService::FindRepositoryRoot(paths.ExecutableDirectory())) {
        candidates.push_back(*repo / "Engine" / "Config" / "Themes" / (theme + ".json"));
    }

    std::error_code ec;
    std::filesystem::path best;
    std::filesystem::file_time_type bestTime{};
    for (const auto& candidate : candidates) {
        if (!std::filesystem::exists(candidate, ec)) {
            continue;
        }
        const auto writeTime = std::filesystem::last_write_time(candidate, ec);
        if (ec) {
            continue;
        }
        if (best.empty() || writeTime > bestTime) {
            best = candidate;
            bestTime = writeTime;
        }
    }
    return best.empty() ? candidates.front() : best;
}

void AssetCatalogService::SetActiveThemeOverride(std::string themeName) {
    std::lock_guard lock(g_ThemeOverrideMutex);
    g_ThemeOverride = std::move(themeName);
}

std::string AssetCatalogService::GetActiveThemeName(const AssetCatalog& catalog) {
    if (const char* env = std::getenv("WE_THEME"); env && env[0] != '\0') {
        return env;
    }
    {
        std::lock_guard lock(g_ThemeOverrideMutex);
        if (!g_ThemeOverride.empty()) {
            return g_ThemeOverride;
        }
    }
    return catalog.activeTheme.empty() ? "GraphiteDark" : catalog.activeTheme;
}

} // namespace we::runtime::core
