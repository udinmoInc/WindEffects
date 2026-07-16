#include "Services/ProjectService.h"

#include "Util/JsonFile.h"
#include "Util/PathUtils.h"

#include <fstream>

namespace we::programs::welauncher {
namespace {

std::vector<std::string> RequiredProjectFolders() {
    return {
        "Config",
        "Content",
        "Source",
        "Plugins",
        "ProjectSettings",
        "Saved",
        "Intermediate",
        "Binaries",
    };
}

} // namespace

ProjectService::ProjectService(
    EngineDiscoveryService& engines,
    ProjectTemplateRegistry& templates,
    LauncherSettingsStore& settings)
    : m_Engines(engines)
    , m_Templates(templates)
    , m_Settings(settings) {
}

std::optional<ProjectSummary> ProjectService::LoadSummary(const std::filesystem::path& weprojPath) const {
    if (!std::filesystem::exists(weprojPath)) {
        return std::nullopt;
    }

    nlohmann::json json;
    if (!JsonFile::Load(weprojPath, json)) {
        return std::nullopt;
    }

    ProjectSummary summary{};
    summary.weprojPath = PathUtils::ToUtf8(weprojPath);
    summary.projectRoot = PathUtils::ToUtf8(weprojPath.parent_path());
    summary.descriptor = json.get<WeProjectDescriptor>();
    summary.compatible = ValidateCompatibility(summary.descriptor, summary.compatibilityMessage);
    return summary;
}

std::vector<ProjectSummary> ProjectService::LoadRecentSummaries() const {
    std::vector<ProjectSummary> out;
    for (const auto& path : m_Settings.RecentProjects()) {
        if (auto summary = LoadSummary(PathUtils::FromUtf8(path))) {
            out.push_back(*summary);
        }
    }
    return out;
}

bool ProjectService::ValidateCompatibility(const WeProjectDescriptor& descriptor, std::string& outMessage) const {
    if (!m_Engines.HasEngine()) {
        outMessage = "No engine installation detected.";
        return false;
    }
    if (descriptor.schemaVersion != 1) {
        outMessage = "Unsupported project schema.";
        return false;
    }

    const auto& currentVersion = m_Engines.Current().engineVersion;
    if (descriptor.engineVersion.empty() || currentVersion.empty()) {
        outMessage = "Engine version unknown.";
        return true;
    }

    const auto majorOf = [](const std::string& v) -> int {
        const auto dot = v.find('.');
        if (dot == std::string::npos) {
            return 0;
        }
        try {
            return std::stoi(v.substr(0, dot));
        } catch (...) {
            return 0;
        }
    };

    if (majorOf(descriptor.engineVersion) != majorOf(currentVersion)) {
        outMessage = "Major engine version mismatch (project " + descriptor.engineVersion
            + ", installed " + currentVersion + ").";
        return false;
    }
    if (descriptor.engineVersion != currentVersion) {
        outMessage = "Minor engine version differs — project may need migration.";
        return true;
    }
    outMessage = "Compatible with installed engine.";
    return true;
}

bool ProjectService::WriteDescriptor(const std::filesystem::path& weprojPath, const WeProjectDescriptor& descriptor) const {
    nlohmann::json json = descriptor;
    return JsonFile::Save(weprojPath, json);
}

bool ProjectService::EnsureProjectLayout(const std::filesystem::path& projectRoot, const std::string& projectName) const {
    std::error_code ec;
    for (const auto& folder : RequiredProjectFolders()) {
        std::filesystem::create_directories(projectRoot / folder, ec);
    }

    const auto sourceModule = projectRoot / "Source" / projectName;
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

    const auto defaultEngine = projectRoot / "Config" / "DefaultEngine.ini";
    if (!std::filesystem::exists(defaultEngine)) {
        std::ofstream file(defaultEngine);
        file << "[/Script/EngineSettings]\n"
             << "GameName=" << projectName << "\n";
    }

    const auto defaultGame = projectRoot / "Config" / "DefaultGame.ini";
    if (!std::filesystem::exists(defaultGame)) {
        std::ofstream file(defaultGame);
        file << "[/Script/EngineSettings]\n"
             << "ProjectID=" << projectName << "\n";
    }

    const auto projectSettings = projectRoot / "ProjectSettings" / "ProjectSettings.json";
    if (!std::filesystem::exists(projectSettings)) {
        nlohmann::json settings = {
            { "projectName", projectName },
            { "startupMap", "" },
            { "targetPlatform", "Windows" },
        };
        (void)JsonFile::Save(projectSettings, settings);
    }

    const auto keepContent = projectRoot / "Content" / ".keep";
    if (!std::filesystem::exists(keepContent)) {
        std::ofstream(keepContent) << "";
    }

    return true;
}

void ProjectService::CopyTemplateTree(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    const std::string& projectName) const
{
    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(source, ec)) {
        if (ec) {
            break;
        }
        const auto rel = std::filesystem::relative(entry.path(), source, ec);
        if (ec) {
            continue;
        }
        if (rel.filename() == "template.json") {
            continue;
        }

        std::string relUtf8 = rel.generic_string();
        relUtf8 = PathUtils::ReplaceTokens(relUtf8, projectName);
        const auto targetPath = destination / relUtf8;

        if (entry.is_directory()) {
            std::filesystem::create_directories(targetPath, ec);
            continue;
        }

        std::filesystem::create_directories(targetPath.parent_path(), ec);
        std::ifstream in(entry.path(), std::ios::binary);
        std::ofstream out(targetPath, std::ios::binary);
        if (!in || !out) {
            continue;
        }

        std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        const auto ext = PathUtils::ToUtf8(entry.path().extension());
        if (ext == ".ini" || ext == ".json" || ext == ".cpp" || ext == ".h" || ext == ".cs" || ext == ".md" || ext.empty()) {
            contents = PathUtils::ReplaceTokens(contents, projectName);
        }
        out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    }
}

bool ProjectService::MaterializeTemplate(
    const ProjectTemplateInfo& tmpl,
    const std::filesystem::path& projectRoot,
    const std::string& projectName) const
{
    if (!tmpl.templateRoot.empty() && std::filesystem::exists(tmpl.templateRoot)) {
        CopyTemplateTree(tmpl.templateRoot, projectRoot, projectName);
        return true;
    }
    return EnsureProjectLayout(projectRoot, projectName);
}

ProjectOperationResult ProjectService::CreateProject(
    const std::string& displayName,
    const std::string& templateId,
    const std::filesystem::path& parentDirectory)
{
    ProjectOperationResult result{};
    if (!m_Engines.HasEngine()) {
        result.message = "Engine installation not found.";
        return result;
    }

    const std::string projectName = PathUtils::SanitizeProjectName(displayName);
    const auto projectRoot = parentDirectory / projectName;
    if (std::filesystem::exists(projectRoot)) {
        result.message = "Project directory already exists.";
        return result;
    }
    if (PathUtils::IsPathInsideEngineInstall(projectRoot, m_Engines.Current().engineRoot)) {
        result.message = "Projects must be created outside the engine installation.";
        return result;
    }

    const ProjectTemplateInfo* tmpl = m_Templates.Find(templateId);
    if (!tmpl) {
        result.message = "Unknown template: " + templateId;
        return result;
    }

    std::error_code ec;
    std::filesystem::create_directories(projectRoot, ec);
    if (ec) {
        result.message = "Failed to create project directory.";
        return result;
    }

    if (!MaterializeTemplate(*tmpl, projectRoot, projectName)) {
        result.message = "Failed to materialize template.";
        return result;
    }
    EnsureProjectLayout(projectRoot, projectName);

    WeProjectDescriptor descriptor{};
    descriptor.projectName = projectName;
    descriptor.displayName = displayName.empty() ? projectName : displayName;
    descriptor.templateId = templateId;
    descriptor.engineVersion = m_Engines.Current().engineVersion;
    descriptor.engineRoot = PathUtils::ToUtf8(m_Engines.Current().engineRoot);
    descriptor.createdUtc = PathUtils::GetUtcNowIso8601();
    descriptor.lastOpenedUtc = descriptor.createdUtc;
    descriptor.modules = { projectName };

    const auto weprojPath = projectRoot / (projectName + ".weproj");
    if (!WriteDescriptor(weprojPath, descriptor)) {
        result.message = "Failed to write project descriptor.";
        return result;
    }

    m_Settings.TouchRecent(PathUtils::ToUtf8(weprojPath));
    m_Settings.Save();

    result.success = true;
    result.weprojPath = PathUtils::ToUtf8(weprojPath);
    result.message = "Project created.";
    return result;
}

ProjectOperationResult ProjectService::OpenProject(const std::filesystem::path& weprojPath) {
    ProjectOperationResult result{};
    auto summary = LoadSummary(weprojPath);
    if (!summary) {
        result.message = "Invalid project file.";
        return result;
    }
    if (!summary->compatible) {
        result.message = summary->compatibilityMessage;
        return result;
    }

    WeProjectDescriptor descriptor = summary->descriptor;
    descriptor.lastOpenedUtc = PathUtils::GetUtcNowIso8601();
    (void)WriteDescriptor(weprojPath, descriptor);

    m_Settings.TouchRecent(PathUtils::ToUtf8(weprojPath));
    m_Settings.Save();

    result.success = true;
    result.weprojPath = PathUtils::ToUtf8(weprojPath);
    result.message = "Project opened.";
    return result;
}

ProjectOperationResult ProjectService::CloneProject(const std::filesystem::path& sourceWeproj, const std::string& newDisplayName) {
    ProjectOperationResult result{};
    auto summary = LoadSummary(sourceWeproj);
    if (!summary) {
        result.message = "Source project invalid.";
        return result;
    }

    const std::string newName = PathUtils::SanitizeProjectName(newDisplayName);
    const auto destRoot = PathUtils::FromUtf8(summary->projectRoot).parent_path() / newName;
    if (std::filesystem::exists(destRoot)) {
        result.message = "Destination already exists.";
        return result;
    }

    std::error_code ec;
    std::filesystem::copy(
        PathUtils::FromUtf8(summary->projectRoot),
        destRoot,
        std::filesystem::copy_options::recursive,
        ec);
    if (ec) {
        result.message = "Clone failed: " + ec.message();
        return result;
    }

    const auto oldWeproj = destRoot / (summary->descriptor.projectName + ".weproj");
    if (std::filesystem::exists(oldWeproj)) {
        std::filesystem::remove(oldWeproj, ec);
    }

    WeProjectDescriptor descriptor = summary->descriptor;
    descriptor.projectName = newName;
    descriptor.displayName = newDisplayName;
    descriptor.createdUtc = PathUtils::GetUtcNowIso8601();
    descriptor.lastOpenedUtc = descriptor.createdUtc;

    const auto newWeproj = destRoot / (newName + ".weproj");
    if (!WriteDescriptor(newWeproj, descriptor)) {
        result.message = "Failed to write cloned descriptor.";
        return result;
    }

    m_Settings.TouchRecent(PathUtils::ToUtf8(newWeproj));
    m_Settings.Save();

    result.success = true;
    result.weprojPath = PathUtils::ToUtf8(newWeproj);
    result.message = "Project cloned.";
    return result;
}

ProjectOperationResult ProjectService::RenameProject(const std::filesystem::path& weprojPath, const std::string& newDisplayName) {
    ProjectOperationResult result{};
    auto summary = LoadSummary(weprojPath);
    if (!summary) {
        result.message = "Project invalid.";
        return result;
    }

    const std::string newName = PathUtils::SanitizeProjectName(newDisplayName);
    const auto oldRoot = PathUtils::FromUtf8(summary->projectRoot);
    const auto newRoot = oldRoot.parent_path() / newName;
    if (newRoot != oldRoot && std::filesystem::exists(newRoot)) {
        result.message = "Target name already exists.";
        return result;
    }

    std::error_code ec;
    if (newRoot != oldRoot) {
        std::filesystem::rename(oldRoot, newRoot, ec);
        if (ec) {
            result.message = "Rename folder failed.";
            return result;
        }
    }

    const auto newWeproj = newRoot / (newName + ".weproj");
    if (newWeproj != weprojPath) {
        if (std::filesystem::exists(weprojPath)) {
            std::filesystem::remove(weprojPath, ec);
        }
    }

    WeProjectDescriptor descriptor = summary->descriptor;
    descriptor.projectName = newName;
    descriptor.displayName = newDisplayName;
    if (!WriteDescriptor(newWeproj, descriptor)) {
        result.message = "Failed to update descriptor.";
        return result;
    }

    m_Settings.RemoveRecent(PathUtils::ToUtf8(weprojPath));
    m_Settings.TouchRecent(PathUtils::ToUtf8(newWeproj));
    m_Settings.Save();

    result.success = true;
    result.weprojPath = PathUtils::ToUtf8(newWeproj);
    result.message = "Project renamed.";
    return result;
}

ProjectOperationResult ProjectService::DeleteProject(const std::filesystem::path& weprojPath) {
    ProjectOperationResult result{};
    auto summary = LoadSummary(weprojPath);
    if (!summary) {
        result.message = "Project invalid.";
        return result;
    }

    std::error_code ec;
    std::filesystem::remove_all(PathUtils::FromUtf8(summary->projectRoot), ec);
    if (ec) {
        result.message = "Delete failed.";
        return result;
    }

    m_Settings.RemoveRecent(PathUtils::ToUtf8(weprojPath));
    m_Settings.Save();

    result.success = true;
    result.message = "Project deleted.";
    return result;
}

ProjectOperationResult ProjectService::RepairProject(const std::filesystem::path& weprojPath) {
    ProjectOperationResult result{};
    auto summary = LoadSummary(weprojPath);
    if (!summary) {
        result.message = "Project invalid.";
        return result;
    }

    if (!EnsureProjectLayout(PathUtils::FromUtf8(summary->projectRoot), summary->descriptor.projectName)) {
        result.message = "Repair failed.";
        return result;
    }

    result.success = true;
    result.weprojPath = PathUtils::ToUtf8(weprojPath);
    result.message = "Project repaired.";
    return result;
}

} // namespace we::programs::welauncher
