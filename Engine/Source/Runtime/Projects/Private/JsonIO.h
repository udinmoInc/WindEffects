#pragma once

#include "Projects/Export.h"

#include <filesystem>
#include <nlohmann/json.h>
#include <optional>

namespace we::projects {

class JsonIO {
public:
    static bool Load(const std::filesystem::path& path, nlohmann::json& out);
    static bool Save(const std::filesystem::path& path, const nlohmann::json& data);
    static std::optional<nlohmann::json> TryLoad(const std::filesystem::path& path);
};

} // namespace we::projects
