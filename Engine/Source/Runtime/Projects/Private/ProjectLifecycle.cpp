#include "Projects/ProjectLifecycle.h"

#include "Projects/EngineContext.h"
#include "JsonIO.h"

#include "Core/Logger.h"
#include "Core/Paths.h"

#include <chrono>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace we::projects {
namespace {

int MajorVersion(const std::string& version) {
    const auto dot = version.find('.');
    const std::string head = dot == std::string::npos ? version : version.substr(0, dot);
    try {
        return std::stoi(head);
    } catch (...) {
        return 0;
    }
}

void CopyTree(
    const std::filesystem::path& source,
    const std::filesystem::path& destination) {
    std::error_code ec;
    std::filesystem::create_directories(destination, ec);
    for (const auto& entry : std::filesystem::recursive_directory_iterator(source, ec)) {
        if (ec) {
            break;
        }
        const auto rel = std::filesystem::relative(entry.path(), source, ec);
        if (ec) {
            continue;
        }
        const auto dest = destination / rel;
        if (entry.is_directory(ec)) {
            std::filesystem::create_directories(dest, ec);
        } else if (entry.is_regular_file(ec)) {
            std::filesystem::create_directories(dest.parent_path(), ec);
            std::filesystem::copy_file(entry.path(), dest, std::filesystem::copy_options::overwrite_existing, ec);
        }
    }
}

std::string WriteStartupMapStub(const std::filesystem::path& projectRoot, const std::string& projectName) {
    const auto mapsDir = projectRoot / we::core::layout::kContent / "Maps";
    std::error_code ec;
    std::filesystem::create_directories(mapsDir, ec);
    const auto mapPath = mapsDir / "Startup.scene";
    if (!std::filesystem::exists(mapPath)) {
        std::ofstream out(mapPath);
        out << "{\n"
            << "  \"type\": \"Scene\",\n"
            << "  \"name\": \"Startup\",\n"
            << "  \"project\": \"" << projectName << "\"\n"
            << "}\n";
    }
    return std::string(we::core::layout::kContent) + "/Maps/Startup.scene";
}

} // namespace

std::string ProjectLifecycle::NowUtc() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const std::time_t t = clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string ProjectLifecycle::SanitizeProjectName(const std::string& displayName) {
    std::string out;
    out.reserve(displayName.size());
    for (unsigned char ch : displayName) {
        if (std::isalnum(ch)) {
            out.push_back(static_cast<char>(ch));
        } else if (ch == ' ' || ch == '-' || ch == '_') {
            if (out.empty() || out.back() == '_') {
                continue;
            }
            out.push_back('_');
        }
    }
    while (!out.empty() && out.back() == '_') {
        out.pop_back();
    }
    if (out.empty()) {
        out = "NewProject";
    }
    if (std::isdigit(static_cast<unsigned char>(out.front()))) {
        out.insert(out.begin(), '_');
    }
    return out;
}

std::optional<WeProjectDescriptor> ProjectLifecycle::ReadDescriptor(
    const std::filesystem::path& weprojPath) {
    const auto json = JsonIO::TryLoad(weprojPath);
    if (!json) {
        return std::nullopt;
    }
    try {
        return json->get<WeProjectDescriptor>();
    } catch (const std::exception& e) {
        HE_ERROR(std::string("[ProjectLifecycle] Invalid .weproj: ") + e.what());
        return std::nullopt;
    }
}

bool ProjectLifecycle::WriteDescriptor(
    const std::filesystem::path& weprojPath,
    const WeProjectDescriptor& descriptor) {
    nlohmann::json json = descriptor;
    return JsonIO::Save(weprojPath, json);
}

ProjectValidationResult ProjectLifecycle::Validate(
    const WeProjectDescriptor& descriptor,
    const std::string& installedEngineVersion) {
    ProjectValidationResult result{};
    if (descriptor.schemaVersion < 1) {
        result.message = "Unsupported project schema.";
        return result;
    }
    if (descriptor.schemaVersion > 1) {
        result.needsUpgrade = true;
        result.message = "Project schema is newer than this editor supports.";
        return result;
    }
    if (descriptor.projectName.empty()) {
        result.message = "Project name is missing.";
        return result;
    }

    if (installedEngineVersion.empty() || descriptor.engineVersion.empty()) {
        result.ok = true;
        result.message = "Engine version unknown — open with caution.";
        return result;
    }

    if (MajorVersion(descriptor.engineVersion) != MajorVersion(installedEngineVersion)) {
        result.message = "Major engine version mismatch (project "
            + descriptor.engineVersion + ", installed " + installedEngineVersion + ").";
        result.needsUpgrade = true;
        return result;
    }

    if (descriptor.engineVersion != installedEngineVersion) {
        result.ok = true;
        result.needsUpgrade = true;
        result.message = "Minor engine version differs — project may need upgrade.";
        return result;
    }

    result.ok = true;
    result.message = "Compatible with installed engine.";
    return result;
}

ProjectValidationResult ProjectLifecycle::ValidateProjectPath(
    const std::filesystem::path& weprojPath,
    const std::string& installedEngineVersion) {
    ProjectValidationResult result{};
    if (!std::filesystem::exists(weprojPath)) {
        result.message = "Project file not found.";
        return result;
    }
    const auto ext = weprojPath.extension().string();
    if (ext != ".weproj" && ext != ".weproject") {
        result.message = "Not a WindEffects project file (.weproj).";
        return result;
    }

    const auto descriptor = ReadDescriptor(weprojPath);
    if (!descriptor) {
        result.message = "Failed to parse project file.";
        return result;
    }

    result = Validate(*descriptor, installedEngineVersion);

    const auto projectRoot = weprojPath.parent_path();
    const std::string contentRel = descriptor->contentDirectory.empty()
        ? we::core::layout::kContent
        : descriptor->contentDirectory;
    if (!std::filesystem::exists(projectRoot / contentRel)) {
        result.ok = false;
        result.message = "Content directory missing: " + contentRel;
        return result;
    }

    // Soft SDK warning — editor can still open.
    if (!std::filesystem::exists(projectRoot / we::core::layout::kConfig)) {
        result.missingSdk = true;
        if (result.message.find("Compatible") != std::string::npos) {
            result.message = "Project Config folder missing — some settings may be unavailable.";
        }
    }

    return result;
}

bool ProjectLifecycle::EnsureProjectLayout(
    const std::filesystem::path& projectRoot,
    const std::string& projectName) {
    const std::filesystem::path folders[] = {
        we::core::layout::kConfig,
        we::core::layout::kContent,
        std::filesystem::path(we::core::layout::kContent) / "Maps",
        we::core::layout::kSource,
        we::core::layout::kPlugins,
        "ProjectSettings",
        we::core::layout::kSaved,
        we::core::layout::kIntermediate,
        we::core::layout::kBinaries,
    };

    std::error_code ec;
    for (const auto& folder : folders) {
        std::filesystem::create_directories(projectRoot / folder, ec);
    }

    const auto sourceModule = projectRoot / we::core::layout::kSource / projectName;
    std::filesystem::create_directories(sourceModule, ec);

    const auto buildCs = sourceModule / (projectName + ".Build.cs");
    if (!std::filesystem::exists(buildCs)) {
        std::ofstream file(buildCs);
        file << "using IgniteBT.BuildSystem;\n\n"
             << "public class " << projectName << " : ModuleRules\n"
             << "{\n"
             << "    public " << projectName << "(ModuleContext context) : base(context)\n"
             << "    {\n"
             << "        Type = ModuleType.GameModule;\n"
             << "        PublicDependencies.Add(\"Core\");\n"
             << "        PublicDependencies.Add(\"Engine\");\n"
             << "    }\n"
             << "}\n";
    }

    const auto cpp = sourceModule / (projectName + ".cpp");
    if (!std::filesystem::exists(cpp)) {
        std::ofstream file(cpp);
        file << "#include \"Core/Logger.h\"\n\n"
             << "void WE_GameModule_" << projectName << "_Startup()\n"
             << "{\n"
             << "    HE_INFO(\"[" << projectName << "] game module loaded\");\n"
             << "}\n";
    }

    const auto defaultEngine = projectRoot / we::core::layout::kConfig / "DefaultEngine.ini";
    if (!std::filesystem::exists(defaultEngine)) {
        std::ofstream file(defaultEngine);
        file << "[/Script/EngineSettings]\n"
             << "GameName=" << projectName << "\n";
    }

    const auto defaultGame = projectRoot / we::core::layout::kConfig / "DefaultGame.ini";
    if (!std::filesystem::exists(defaultGame)) {
        std::ofstream file(defaultGame);
        file << "[/Script/EngineSettings]\n"
             << "ProjectID=" << projectName << "\n";
    }

    const auto projectSettings = projectRoot / "ProjectSettings" / "ProjectSettings.json";
    if (!std::filesystem::exists(projectSettings)) {
        nlohmann::json settings = {
            { "projectName", projectName },
            { "startupMap", std::string(we::core::layout::kContent) + "/Maps/Startup.scene" },
            { "defaultGameMode", "" },
            { "targetPlatform", "Windows" },
        };
        (void)JsonIO::Save(projectSettings, settings);
    }

    const auto keepContent = projectRoot / we::core::layout::kContent / ".keep";
    if (!std::filesystem::exists(keepContent)) {
        std::ofstream(keepContent) << "";
    }

    return true;
}

ProjectOperationResult ProjectLifecycle::CreateNewProject(const ProjectCreateRequest& request) {
    ProjectOperationResult result{};
    if (request.displayName.empty()) {
        result.message = "Project name is required.";
        return result;
    }
    if (request.parentDirectory.empty()) {
        result.message = "Parent directory is required.";
        return result;
    }

    const std::string projectName = SanitizeProjectName(request.displayName);
    const auto projectRoot = request.parentDirectory / projectName;
    std::error_code ec;
    if (std::filesystem::exists(projectRoot, ec)) {
        result.message = "A folder already exists at: " + projectRoot.string();
        return result;
    }

    if (!EnsureProjectLayout(projectRoot, projectName)) {
        result.message = "Failed to create project folders.";
        return result;
    }

    const std::string startupMap = WriteStartupMapStub(projectRoot, projectName);

    WeProjectDescriptor descriptor{};
    descriptor.schemaVersion = 1;
    descriptor.projectName = projectName;
    descriptor.displayName = request.displayName;
    descriptor.templateId = request.templateId.empty() ? "Blank" : request.templateId;
    descriptor.engineVersion = request.engineVersion.empty()
        ? (EngineContext::Get().IsInitialized() ? EngineContext::Get().EngineVersion() : std::string{})
        : request.engineVersion;
    descriptor.engineRoot = request.engineRoot.empty()
        ? (EngineContext::Get().IsInitialized() ? EngineContext::Get().EngineRoot().string() : std::string{})
        : request.engineRoot;
    descriptor.contentDirectory = we::core::layout::kContent;
    descriptor.startupMap = startupMap;
    descriptor.defaultGameMode.clear();
    descriptor.modules = { projectName };
    descriptor.plugins = {};
    descriptor.targetPlatforms = { "Windows" };
    descriptor.settings = {
        { "startupMap", startupMap },
        { "defaultGameMode", "" },
    };
    descriptor.createdUtc = NowUtc();
    descriptor.lastOpenedUtc = descriptor.createdUtc;

    const auto weprojPath = projectRoot / (projectName + ".weproj");
    if (!WriteDescriptor(weprojPath, descriptor)) {
        result.message = "Failed to write .weproj file.";
        return result;
    }

    // Optional template content copy (Editor EngineContext, or engineRoot from request).
    std::filesystem::path templatesRoot;
    if (EngineContext::Get().IsInitialized()) {
        templatesRoot = EngineContext::Get().TemplatesRoot();
    } else if (!request.engineRoot.empty()) {
        templatesRoot = std::filesystem::path(request.engineRoot) / we::core::layout::kEngine
            / we::core::layout::kTemplates / we::core::layout::kProjects;
    }
    if (!templatesRoot.empty()) {
        const auto templateRoot = templatesRoot / descriptor.templateId;
        if (std::filesystem::exists(templateRoot / we::core::layout::kContent)) {
            CopyTree(templateRoot / we::core::layout::kContent, projectRoot / we::core::layout::kContent);
            (void)WriteStartupMapStub(projectRoot, projectName);
        }
    }

    result.success = true;
    result.message = "Project created.";
    result.weprojPath = weprojPath;
    result.projectRoot = projectRoot;
    return result;
}

ProjectOperationResult ProjectLifecycle::CloneProject(
    const std::filesystem::path& sourceWeproj,
    const std::string& newDisplayName,
    const std::filesystem::path& parentDirectory) {
    ProjectOperationResult result{};
    const auto sourceDesc = ReadDescriptor(sourceWeproj);
    if (!sourceDesc) {
        result.message = "Source project could not be read.";
        return result;
    }

    const std::string projectName = SanitizeProjectName(newDisplayName);
    const auto destRoot = parentDirectory / projectName;
    std::error_code ec;
    if (std::filesystem::exists(destRoot, ec)) {
        result.message = "Destination already exists.";
        return result;
    }

    CopyTree(sourceWeproj.parent_path(), destRoot);

    const auto oldWeproj = destRoot / (sourceDesc->projectName + ".weproj");
    const auto newWeproj = destRoot / (projectName + ".weproj");
    if (std::filesystem::exists(oldWeproj) && oldWeproj != newWeproj) {
        std::filesystem::rename(oldWeproj, newWeproj, ec);
    }

    WeProjectDescriptor descriptor = *sourceDesc;
    descriptor.projectName = projectName;
    descriptor.displayName = newDisplayName;
    descriptor.modules = { projectName };
    descriptor.createdUtc = NowUtc();
    descriptor.lastOpenedUtc = descriptor.createdUtc;
    if (!WriteDescriptor(newWeproj, descriptor)) {
        result.message = "Cloned files but failed to write new .weproj.";
        return result;
    }

    result.success = true;
    result.message = "Project cloned.";
    result.weprojPath = newWeproj;
    result.projectRoot = destRoot;
    return result;
}

ProjectOperationResult ProjectLifecycle::UpgradeProject(
    const std::filesystem::path& weprojPath,
    const std::string& targetEngineVersion,
    const std::string& targetEngineRoot) {
    ProjectOperationResult result{};
    auto descriptor = ReadDescriptor(weprojPath);
    if (!descriptor) {
        result.message = "Failed to read project for upgrade.";
        return result;
    }

    descriptor->engineVersion = targetEngineVersion;
    descriptor->engineRoot = targetEngineRoot;
    descriptor->schemaVersion = 1;
    descriptor->lastOpenedUtc = NowUtc();

    if (descriptor->contentDirectory.empty()) {
        descriptor->contentDirectory = we::core::layout::kContent;
    }
    if (descriptor->targetPlatforms.empty()) {
        descriptor->targetPlatforms = { "Windows" };
    }

    (void)EnsureProjectLayout(weprojPath.parent_path(), descriptor->projectName);

    if (!WriteDescriptor(weprojPath, *descriptor)) {
        result.message = "Failed to write upgraded project file.";
        return result;
    }

    result.success = true;
    result.message = "Project upgraded to engine " + targetEngineVersion + ".";
    result.weprojPath = weprojPath;
    result.projectRoot = weprojPath.parent_path();
    return result;
}

} // namespace we::projects
