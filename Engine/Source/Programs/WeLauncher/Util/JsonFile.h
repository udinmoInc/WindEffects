#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <nlohmann/json.h>

namespace we::programs::welauncher {

class JsonFile {
public:
    static bool Load(const std::filesystem::path& path, nlohmann::json& out);
    static bool Save(const std::filesystem::path& path, const nlohmann::json& data);
    static std::optional<nlohmann::json> TryLoad(const std::filesystem::path& path);
};

} // namespace we::programs::welauncher
