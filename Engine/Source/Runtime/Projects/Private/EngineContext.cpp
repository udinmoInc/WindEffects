#include "Projects/EngineContext.h"

#include "Core/Logger.h"
#include "Core/Paths.h"

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
    return we::core::PathService::FindEngineRoot(startDirectory);
}

void EngineContext::Initialize(const std::filesystem::path& executableDirectory) {
    m_ExecutableDirectory = we::core::PathService::Absolute(executableDirectory);
    m_EngineRoot.clear();
    m_EngineContentRoot.clear();
    m_EngineConfigRoot.clear();
    m_EngineShadersRoot.clear();
    m_EngineBinariesRoot.clear();
    m_TemplatesRoot.clear();
    m_EngineVersion = "0.0.0";

    auto& paths = we::core::PathService::Get();

    if (const auto root = FindEngineRoot(m_ExecutableDirectory)) {
        m_EngineRoot = *root;
    } else {
        m_EngineRoot = m_ExecutableDirectory;
        HE_WARN("[EngineContext] WindEffects.engine not found; using executable directory as engine root.");
    }

    const auto descriptorPath = m_EngineRoot / we::core::layout::kEngineMarker;
    if (std::filesystem::exists(descriptorPath)) {
        const auto values = ParseIni(descriptorPath);
        if (values.count("engine.version")) {
            m_EngineVersion = values.at("engine.version");
        }
        if (values.count("TemplatesRoot")) {
            m_TemplatesRoot = we::core::PathService::ResolveRelative(
                m_EngineRoot,
                values.at("TemplatesRoot"));
        } else {
            m_TemplatesRoot = m_EngineRoot / we::core::layout::kEngine
                / we::core::layout::kTemplates / we::core::layout::kProjects;
        }
    } else {
        m_TemplatesRoot = m_EngineRoot / we::core::layout::kEngine
            / we::core::layout::kTemplates / we::core::layout::kProjects;
    }

    m_EngineContentRoot = m_EngineRoot / we::core::layout::kEngine / we::core::layout::kContent;
    m_EngineConfigRoot = m_EngineRoot / we::core::layout::kEngine / we::core::layout::kConfig;
    m_EngineShadersRoot = m_EngineRoot / we::core::layout::kEngine / we::core::layout::kShaders;
    m_EngineBinariesRoot = m_EngineRoot / we::core::layout::kEngine / we::core::layout::kBinaries;
    m_Initialized = true;

    we::core::PathService::Configuration config;
    config.executableDirectory = m_ExecutableDirectory;
    config.engineRoot = m_EngineRoot;
    paths.Configure(config);

    const auto layout = paths.ValidateLayout(false);
    if (!layout.ok) {
        HE_WARN("[EngineContext] Layout validation failed: " + layout.summary);
        for (const auto& check : layout.checks) {
            if (check.required && !check.result.ok) {
                HE_WARN("[EngineContext]   " + check.name + ": " + check.result.message);
            }
        }
    } else {
        HE_INFO("[EngineContext] Layout validation passed.");
    }

    HE_INFO("[EngineContext] Engine root: " + we::core::PathService::ToGeneric(m_EngineRoot));
    HE_INFO("[EngineContext] Engine version: " + m_EngineVersion);
}

void EngineContext::Shutdown() {
    m_Initialized = false;
    m_ExecutableDirectory.clear();
    m_EngineRoot.clear();
    m_EngineContentRoot.clear();
    m_EngineConfigRoot.clear();
    m_EngineShadersRoot.clear();
    m_EngineBinariesRoot.clear();
    m_TemplatesRoot.clear();
    m_EngineVersion = "0.0.0";
    we::core::PathService::Get().Shutdown();
}

} // namespace we::projects
