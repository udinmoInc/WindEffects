#include "Services/ProjectTemplateRegistry.h"

#include "Util/JsonFile.h"

#include <algorithm>

namespace we::programs::welauncher {

void ProjectTemplateRegistry::RegisterBuiltInFallbacks() {
    const auto add = [&](const char* id, const char* name, const char* desc) {
        ProjectTemplateInfo info{};
        info.id = id;
        info.displayName = name;
        info.description = desc;
        info.category = "Games";
        m_Templates.push_back(std::move(info));
    };
    add("Blank", "Blank", "Empty folders with default config only.");
    add("FirstPerson", "First Person", "First-person character starter.");
    add("ThirdPerson", "Third Person", "Third-person character starter.");
    add("TopDown", "Top Down", "Top-down gameplay starter.");
    add("Vehicle", "Vehicle", "Vehicle physics starter.");
    add("VR", "VR", "VR template with XR hooks.");
    add("TwoD", "2D", "2D gameplay starter.");
    add("Empty", "Empty Project", "Minimal project with no starter gameplay code.");
}

void ProjectTemplateRegistry::LoadFromDisk(const std::filesystem::path& templatesRoot) {
    if (!std::filesystem::exists(templatesRoot)) {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(templatesRoot)) {
        if (!entry.is_directory()) {
            continue;
        }
        const auto manifestPath = entry.path() / "template.json";
        if (!std::filesystem::exists(manifestPath)) {
            continue;
        }
        nlohmann::json json;
        if (!JsonFile::Load(manifestPath, json)) {
            continue;
        }

        ProjectTemplateInfo info{};
        info.id = json.value("id", entry.path().filename().string());
        info.displayName = json.value("displayName", info.id);
        info.description = json.value("description", std::string{});
        info.category = json.value("category", "Games");
        info.templateRoot = entry.path();

        auto it = std::find_if(m_Templates.begin(), m_Templates.end(), [&](const ProjectTemplateInfo& t) {
            return t.id == info.id;
        });
        if (it != m_Templates.end()) {
            *it = info;
        } else {
            m_Templates.push_back(std::move(info));
        }
    }
}

void ProjectTemplateRegistry::Load(const EngineDescriptorData& engine) {
    m_Templates.clear();
    RegisterBuiltInFallbacks();
    LoadFromDisk(engine.templatesRoot);
}

const ProjectTemplateInfo* ProjectTemplateRegistry::Find(std::string_view id) const {
    for (const auto& t : m_Templates) {
        if (t.id == id) {
            return &t;
        }
    }
    return nullptr;
}

} // namespace we::programs::welauncher
