// ==============================================================================
// WindEffects — ContentBrowser — SearchController
// Internal implementation for the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Registry/AssetTypes.h"
#include <regex>
#include <string>
#include <vector>

namespace we::editor::contentbrowser {

class SearchController {
public:
    void SetQuery(const std::string& query) { m_Query = query; m_LowerQuery = ToLower(query); }
    const std::string& GetQuery() const { return m_Query; }
    void SetUseRegex(bool enabled) { m_UseRegex = enabled; }

    bool Matches(const AssetRecord& asset) const;

private:
    static std::string ToLower(std::string value);

    std::string m_Query;
    std::string m_LowerQuery;
    bool m_UseRegex = false;
};

} // namespace we::editor::contentbrowser
