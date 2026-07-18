#include "JsonIO.h"

#include "Core/Logger.h"

#include <fstream>

namespace we::projects {

bool JsonIO::Load(const std::filesystem::path& path, nlohmann::json& out) {
#if !WE_HAS_NLOHMANN_JSON
    (void)path;
    (void)out;
    return false;
#else
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    try {
        file >> out;
        return true;
    } catch (const std::exception& e) {
        HE_ERROR(std::string("[Projects] Failed to parse JSON: ") + path.string() + " — " + e.what());
        return false;
    }
#endif
}

bool JsonIO::Save(const std::filesystem::path& path, const nlohmann::json& data) {
#if !WE_HAS_NLOHMANN_JSON
    (void)path;
    (void)data;
    return false;
#else
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    file << data.dump(2);
    return static_cast<bool>(file);
#endif
}

std::optional<nlohmann::json> JsonIO::TryLoad(const std::filesystem::path& path) {
    nlohmann::json json;
    if (!Load(path, json)) {
        return std::nullopt;
    }
    return json;
}

} // namespace we::projects
