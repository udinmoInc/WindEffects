#include "Util/JsonFile.h"

#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

#include <exception>
#include <fstream>
#include <sstream>
#include <string>

namespace we::programs::welauncher {
namespace {

std::ifstream OpenInput(const std::filesystem::path& path) {
#if defined(_WIN32)
    return std::ifstream(path.wstring(), std::ios::in | std::ios::binary);
#else
    return std::ifstream(path, std::ios::in | std::ios::binary);
#endif
}

std::ofstream OpenOutput(const std::filesystem::path& path) {
#if defined(_WIN32)
    return std::ofstream(path.wstring(), std::ios::out | std::ios::binary | std::ios::trunc);
#else
    return std::ofstream(path, std::ios::out | std::ios::binary | std::ios::trunc);
#endif
}

} // namespace

bool JsonFile::Load(const std::filesystem::path& path, nlohmann::json& out) {
    std::ifstream file = OpenInput(path);
    if (!file.is_open()) {
        return false;
    }
    try {
        std::ostringstream buffer;
        buffer << file.rdbuf();
        const std::string text = buffer.str();
        if (text.empty()) {
            return false;
        }
        out = nlohmann::json::parse(text, nullptr, true, true);
        return out.is_object() || out.is_array();
    } catch (const std::exception& ex) {
        WE_LOG_WARN(we::LogCategory::General.data(),
            std::string("JsonFile::Load failed for ") + path.string() + ": " + ex.what());
        return false;
    } catch (...) {
        WE_LOG_WARN(we::LogCategory::General.data(),
            std::string("JsonFile::Load failed for ") + path.string());
        return false;
    }
}

bool JsonFile::Save(const std::filesystem::path& path, const nlohmann::json& data) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream file = OpenOutput(path);
    if (!file.is_open()) {
        return false;
    }
    try {
        file << data.dump(2);
        return file.good();
    } catch (const std::exception& ex) {
        WE_LOG_WARN(we::LogCategory::General.data(),
            std::string("JsonFile::Save failed for ") + path.string() + ": " + ex.what());
        return false;
    } catch (...) {
        WE_LOG_WARN(we::LogCategory::General.data(),
            std::string("JsonFile::Save failed for ") + path.string());
        return false;
    }
}

std::optional<nlohmann::json> JsonFile::TryLoad(const std::filesystem::path& path) {
    nlohmann::json data;
    if (!Load(path, data)) {
        return std::nullopt;
    }
    return data;
}

} // namespace we::programs::welauncher
