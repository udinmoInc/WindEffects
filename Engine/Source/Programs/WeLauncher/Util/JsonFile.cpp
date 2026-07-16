#include "Util/JsonFile.h"

#include <fstream>

namespace we::programs::welauncher {

bool JsonFile::Load(const std::filesystem::path& path, nlohmann::json& out) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    try {
        file >> out;
        return true;
    } catch (...) {
        return false;
    }
}

bool JsonFile::Save(const std::filesystem::path& path, const nlohmann::json& data) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    file << data.dump(2);
    return file.good();
}

std::optional<nlohmann::json> JsonFile::TryLoad(const std::filesystem::path& path) {
    nlohmann::json data;
    if (!Load(path, data)) {
        return std::nullopt;
    }
    return data;
}

} // namespace we::programs::welauncher
