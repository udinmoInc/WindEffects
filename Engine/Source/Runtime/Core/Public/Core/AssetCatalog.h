// ==============================================================================
// WindEffects — Core — AssetCatalog
// Dynamic asset/theme catalog loaded from Engine/Config/Runtime/asset_catalog.json.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/Export.h"

#include <filesystem>
#include <string>
#include <vector>

namespace we::runtime::core {

struct AssetCatalogEntry {
    std::string name;
    std::string file;
    bool required = false;
};

struct AssetCatalog {
    int schema = 1;
    std::string activeTheme = "GraphiteDark";
    std::vector<AssetCatalogEntry> fonts;
    std::vector<AssetCatalogEntry> shaders;
    std::vector<AssetCatalogEntry> icons;
    AssetCatalogEntry iconAtlasRoot{"Icon_AtlasRoot", "Atlas", false};
    AssetCatalogEntry iconMeta{"Icon_Meta", "Atlas/icons.weiconmeta", false};
};

class CORE_API AssetCatalogService {
public:
    /// Load catalog from PathService config candidates. Falls back to built-in defaults.
    [[nodiscard]] static AssetCatalog Load();

    /// Resolve active theme JSON path from catalog + PathService (newest existing candidate).
    [[nodiscard]] static std::filesystem::path ResolveThemeConfigPath(const AssetCatalog& catalog);

    /// Override active theme name (also honored via WE_THEME env).
    static void SetActiveThemeOverride(std::string themeName);
    [[nodiscard]] static std::string GetActiveThemeName(const AssetCatalog& catalog);
};

} // namespace we::runtime::core
