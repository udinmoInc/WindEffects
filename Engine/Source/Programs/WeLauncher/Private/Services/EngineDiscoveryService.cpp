#include "Services/EngineDiscoveryService.h"

#include "Util/JsonFile.h"
#include "Util/PathUtils.h"

#include <fstream>
#include <unordered_map>

namespace we::programs::welauncher {
namespace {

std::unordered_map<std::string, std::string> ParseIniGlobal(const std::filesystem::path& path) {
    std::unordered_map<std::string, std::string> values;
    std::ifstream file(path);
    if (!file.is_open()) {
        return values;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (line.front() == '[') {
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

bool EngineDiscoveryService::TryLoadDescriptor(const std::filesystem::path& engineRoot, EngineDescriptorData& out) {
    const auto descriptorPath = engineRoot / "WindEffects.engine";
    if (!std::filesystem::exists(descriptorPath)) {
        return false;
    }

    const auto values = ParseIniGlobal(descriptorPath);
    out.engineRoot = engineRoot;
    out.schema = values.count("schema") ? values.at("schema") : "1";
    out.engineVersion = values.count("engine.version") ? values.at("engine.version") : "0.0.0";
    out.programsRoot = PathUtils::ResolveRelative(engineRoot, values.count("ProgramsRoot") ? values.at("ProgramsRoot") : "Engine/Source/Programs");
    out.buildRoot = PathUtils::ResolveRelative(engineRoot, values.count("BuildRoot") ? values.at("BuildRoot") : "Build");
    out.assetsRoot = PathUtils::ResolveRelative(engineRoot, values.count("AssetsRoot") ? values.at("AssetsRoot") : "Assets");
    out.projectsRoot = PathUtils::ResolveRelative(engineRoot, values.count("ProjectsRoot") ? values.at("ProjectsRoot") : "Projects");
    out.templatesRoot = PathUtils::ResolveRelative(engineRoot, values.count("TemplatesRoot") ? values.at("TemplatesRoot") : "Engine/Templates/Projects");
    return true;
}

bool EngineDiscoveryService::Discover(const std::filesystem::path& startDirectory) {
    m_Installed.clear();
    m_Current = {};

    const auto engineRoot = PathUtils::FindEngineRoot(startDirectory);
    if (!engineRoot) {
        return false;
    }

    if (!TryLoadDescriptor(*engineRoot, m_Current)) {
        return false;
    }

    EngineInstallInfo current{};
    current.engineRoot = PathUtils::ToUtf8(m_Current.engineRoot);
    current.engineVersion = m_Current.engineVersion;
    current.displayLabel = "Current (" + m_Current.engineVersion + ")";
    current.buildId = "Development";
    current.sdkStatus = "Checking…";
    current.pluginCount = 0;
    current.updateStatus = "Local development build";
    current.isCurrent = true;
    m_Installed.push_back(current);

    const auto manifestPath = PathUtils::GetLauncherSettingsPath().parent_path() / "engine.json";
    if (const auto manifest = JsonFile::TryLoad(manifestPath)) {
        const std::string manifestRoot = manifest->value("ProjectRoot", std::string{});
        if (!manifestRoot.empty()) {
            EngineDescriptorData extra{};
            if (TryLoadDescriptor(PathUtils::FromUtf8(manifestRoot), extra)) {
                const std::string utf8 = PathUtils::ToUtf8(extra.engineRoot);
                if (utf8 != current.engineRoot) {
                    EngineInstallInfo info{};
                    info.engineRoot = utf8;
                    info.engineVersion = extra.engineVersion;
                    info.displayLabel = "Installed (" + extra.engineVersion + ")";
                    info.buildId = "Development";
                    info.sdkStatus = "Unknown";
                    info.pluginCount = 0;
                    info.updateStatus = "Alternate install";
                    m_Installed.push_back(info);
                }
            }
        }
    }

    return true;
}

std::filesystem::path EngineDiscoveryService::ResolveEditorExecutable(const std::string& buildConfig) const {
    if (m_Current.engineRoot.empty()) {
        return {};
    }

#if defined(_WIN32)
    const char* platformFolder = "Win64";
#else
    const char* platformFolder = "Linux";
#endif

    const auto outputRoot = m_Current.buildRoot / "Output" / platformFolder / buildConfig;
    const std::vector<std::string> candidates = {
        "WindeffectsEditor.exe",
    };

    for (const auto& name : candidates) {
        const auto path = outputRoot / name;
        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    // Fallback: launcher may sit next to editor in config root.
    const auto configRoot = PathUtils::GetExecutableDirectory();
    for (const auto& name : candidates) {
        const auto path = configRoot / name;
        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    return outputRoot / "WindeffectsEditor.exe";
}

} // namespace we::programs::welauncher
