#include "Services/ProjectTemplateRegistry.h"

#include "Util/JsonFile.h"
#include "Util/PathUtils.h"

#include <algorithm>

namespace we::programs::welauncher {
namespace {

ProjectTemplateInfo MakeTemplate(
    const char* id,
    const char* name,
    const char* desc,
    const char* category,
    const char* recommended,
    std::vector<std::string> platforms,
    std::vector<std::string> tags,
    std::vector<std::string> features,
    std::vector<std::string> plugins = {},
    std::vector<std::string> assets = {}) {
    ProjectTemplateInfo info{};
    info.id = id;
    info.displayName = name;
    info.description = desc;
    info.category = category;
    info.recommendedUse = recommended;
    info.platforms = std::move(platforms);
    info.tags = std::move(tags);
    info.features = std::move(features);
    info.plugins = std::move(plugins);
    info.starterAssets = std::move(assets);
    return info;
}

} // namespace

void ProjectTemplateRegistry::RegisterBuiltInFallbacks() {
    m_Templates.push_back(MakeTemplate(
        "Blank", "Blank",
        "Clean project structure with default config and no starter gameplay.",
        "Foundation", "Prototyping and custom pipelines",
        { "Windows", "Linux", "Mac" }, { "Minimal", "Starter" },
        { "Default config", "Module scaffolding", "Asset folders" },
        {}, { "Config", "Content/" }));
    m_Templates.push_back(MakeTemplate(
        "FirstPerson", "First Person",
        "First-person character controller with camera, input, and basic locomotion.",
        "Games", "FPS and immersive exploration",
        { "Windows", "Linux" }, { "Character", "3D", "Action" },
        { "FPS camera", "Character movement", "Input mapping" },
        { "Input", "Gameplay" }, { "Player pawn", "Sample level" }));
    m_Templates.push_back(MakeTemplate(
        "ThirdPerson", "Third Person",
        "Third-person character starter with follow camera and animation hooks.",
        "Games", "Action-adventure and character games",
        { "Windows", "Linux", "Mac" }, { "Character", "3D" },
        { "Follow camera", "Locomotion", "Animation ready" },
        { "Input", "Gameplay" }, { "Character mesh", "Sample level" }));
    m_Templates.push_back(MakeTemplate(
        "TopDown", "Top Down",
        "Top-down camera and click-to-move gameplay foundation.",
        "Games", "Strategy and twin-stick prototypes",
        { "Windows", "Linux" }, { "2.5D", "Strategy" },
        { "Ortho camera", "Click movement", "Nav helpers" },
        { "AI", "Gameplay" }, { "Playable map" }));
    m_Templates.push_back(MakeTemplate(
        "Vehicle", "Vehicle",
        "Vehicle physics starter with chassis setup and camera chase.",
        "Games", "Racing and simulation",
        { "Windows" }, { "Physics", "Vehicles" },
        { "Vehicle physics", "Chase camera", "Input presets" },
        { "Physics", "Gameplay" }, { "Sample vehicle" }));
    m_Templates.push_back(MakeTemplate(
        "VR", "VR",
        "VR template with XR session hooks and stereo-ready viewport setup.",
        "XR", "Headsets and immersive apps",
        { "Windows" }, { "XR", "VR" },
        { "XR bootstrap", "Stereo rendering", "Motion controllers" },
        { "XR", "Input" }, { "VR pawn" }));
    m_Templates.push_back(MakeTemplate(
        "TwoD", "2D",
        "2D gameplay starter with orthographic camera and sprite-friendly content layout.",
        "Games", "Platformers and 2D action",
        { "Windows", "Linux", "Mac" }, { "2D", "Sprites" },
        { "Ortho camera", "2D content layout", "Pixel-friendly defaults" },
        { "Gameplay" }, { "Sample sprites" }));
    m_Templates.push_back(MakeTemplate(
        "Empty", "Empty Project",
        "Absolute minimum project for engine bring-up and tooling tests.",
        "Foundation", "Engine validation and tooling",
        { "Windows", "Linux", "Mac" }, { "Minimal" },
        { "Bare project", "No starter content" }));
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
        info.id = json.value("id", PathUtils::ToUtf8(entry.path().filename()));
        info.displayName = json.value("displayName", info.id);
        info.description = json.value("description", std::string{});
        info.category = json.value("category", "Games");
        info.recommendedUse = json.value("recommendedUse", std::string{});
        info.platforms = json.value("platforms", std::vector<std::string>{ "Windows" });
        info.tags = json.value("tags", std::vector<std::string>{});
        info.features = json.value("features", std::vector<std::string>{});
        info.plugins = json.value("plugins", std::vector<std::string>{});
        info.starterAssets = json.value("starterAssets", std::vector<std::string>{});
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
