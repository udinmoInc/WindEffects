#include "Projects/EngineContext.h"

#include "Core/Logger.h"

#include <cctype>
#include <fstream>
#include <unordered_map>

namespace we::projects {
namespace {

std::unordered_map<std::string, std::string> ParseIni(const std::filesystem::path& path) {
    std::unordered_map<std::string, std::string> values;
    std::ifstream file(path);
    if (!file.is_open()) {
        return values;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line.front() == '[') {
            continue;
        }
        const auto sep = line.find('=');
        if (sep == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, sep);
        std::string value = line.substr(sep + 1);
        while (!key.empty() && std::isspace(static_cast<unsigned char>(key.back()))) {
            key.pop_back();
        }
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
            value.erase(value.begin());
        }
        values[key] = value;
    }
    return values;
}

} // namespace

EngineContext& EngineContext::Get() {
    static EngineContext instance;
    return instance;
}

std::optional<std::filesystem::path> EngineContext::FindEngineRoot(
    const std::filesystem::path& startDirectory) {
    std::error_code ec;
    auto current = std::filesystem::absolute(startDirectory, ec);
    if (ec) {
        current = startDirectory;
    }

    for (int i = 0; i < 12; ++i) {
        if (std::filesystem::exists(current / "WindEffects.engine", ec)) {
            return current;
        }
        if (!current.has_parent_path() || current.parent_path() == current) {
            break;
        }
        current = current.parent_path();
    }
    return std::nullopt;
}

void EngineContext::Initialize(const std::filesystem::path& executableDirectory) {
    m_ExecutableDirectory = executableDirectory;
    m_EngineRoot.clear();
    m_EngineContentRoot.clear();
    m_TemplatesRoot.clear();
    m_EngineVersion = "0.0.0";

    if (const auto root = FindEngineRoot(executableDirectory)) {
        m_EngineRoot = *root;
    } else if (const auto fromCwd = FindEngineRoot(std::filesystem::current_path())) {
        m_EngineRoot = *fromCwd;
    } else {
        // Development layouts often place the editor under Build/Output/<plat>/<cfg>.
        m_EngineRoot = executableDirectory;
        HE_WARN("[EngineContext] WindEffects.engine not found; using executable directory as engine root.");
    }

    const auto descriptorPath = m_EngineRoot / "WindEffects.engine";
    if (std::filesystem::exists(descriptorPath)) {
        const auto values = ParseIni(descriptorPath);
        if (values.count("engine.version")) {
            m_EngineVersion = values.at("engine.version");
        }
        const std::string templatesRel = values.count("TemplatesRoot")
            ? values.at("TemplatesRoot")
            : "Engine/Templates/Projects";
        m_TemplatesRoot = m_EngineRoot / templatesRel;
    } else {
        m_TemplatesRoot = m_EngineRoot / "Engine" / "Templates" / "Projects";
    }

    m_EngineContentRoot = m_EngineRoot / "Engine" / "Content";
    m_Initialized = true;

    HE_INFO("[EngineContext] Engine root: " + m_EngineRoot.string());
    HE_INFO("[EngineContext] Engine version: " + m_EngineVersion);
}

void EngineContext::Shutdown() {
    m_Initialized = false;
    m_ExecutableDirectory.clear();
    m_EngineRoot.clear();
    m_EngineContentRoot.clear();
    m_TemplatesRoot.clear();
    m_EngineVersion = "0.0.0";
}

} // namespace we::projects
