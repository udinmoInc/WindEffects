#include "Services/ProjectService.h"

#include "Util/PathUtils.h"
#include "Core/Paths.h"
#include "Projects/ProjectLifecycle.h"

#include <fstream>
#include <sstream>

namespace we::programs::welauncher {
namespace {


std::uint64_t EstimateDirectoryBytes(const std::filesystem::path& root, std::size_t maxEntries = 400) {
    std::uint64_t total = 0;
    std::size_t seen = 0;
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) {
        return 0;
    }
    const auto options = std::filesystem::directory_options::skip_permission_denied;
    for (std::filesystem::recursive_directory_iterator it(root, options, ec), end;
         it != end && !ec && seen < maxEntries;
         it.increment(ec), ++seen) {
        if (it->is_regular_file(ec)) {
            total += it->file_size(ec);
        }
    }
    return total;
}

std::string JoinPlatforms(const std::vector<std::string>& platforms) {
    if (platforms.empty()) {
        return "Windows";
    }
    std::ostringstream oss;
    for (std::size_t i = 0; i < platforms.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << platforms[i];
    }
    return oss.str();
}

void FillDerivedFields(ProjectSummary& summary, const ProjectTemplateRegistry& templates) {
    if (!summary.compatible) {
        summary.statusLabel = "Incompatible";
    } else if (summary.compatibilityMessage.find("migration") != std::string::npos
        || summary.compatibilityMessage.find("differs") != std::string::npos) {
        summary.statusLabel = "Warning";
    } else {
        summary.statusLabel = "Compatible";
    }

    summary.projectType = summary.descriptor.templateId.empty() ? "Project" : summary.descriptor.templateId;
    summary.platforms = summary.descriptor.targetPlatforms.empty()
        ? "Windows"
        : JoinPlatforms(summary.descriptor.targetPlatforms);
    summary.description.clear();

    if (const auto* tmpl = templates.Find(summary.descriptor.templateId)) {
        summary.projectType = tmpl->displayName.empty() ? tmpl->id : tmpl->displayName;
        summary.platforms = JoinPlatforms(tmpl->platforms);
        summary.description = tmpl->description;
    }

    summary.diskBytes = EstimateDirectoryBytes(PathUtils::FromUtf8(summary.projectRoot));
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
    auto descriptor = we::projects::ProjectLifecycle::ReadDescriptor(weprojPath);
    if (!descriptor) {
        return std::nullopt;
    }

    ProjectSummary summary{};
    summary.weprojPath = PathUtils::ToUtf8(weprojPath);
    summary.projectRoot = PathUtils::ToUtf8(weprojPath.parent_path());
    summary.descriptor = std::move(*descriptor);
    summary.compatible = ValidateCompatibility(summary.descriptor, summary.compatibilityMessage);
    FillDerivedFields(summary, m_Templates);
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
    return we::projects::ProjectLifecycle::WriteDescriptor(weprojPath, descriptor);
}

bool ProjectService::EnsureProjectLayout(const std::filesystem::path& projectRoot, const std::string& projectName) const {
    return we::projects::ProjectLifecycle::EnsureProjectLayout(projectRoot, projectName);
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

    const std::string projectName = we::projects::ProjectLifecycle::SanitizeProjectName(displayName);
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

    const bool hasTemplateTree =
        !tmpl->templateRoot.empty() && std::filesystem::exists(tmpl->templateRoot);

    // Blank / metadata-only templates: ProjectLifecycle owns scaffolding + .weproj write.
    if (!hasTemplateTree) {
        we::projects::ProjectCreateRequest request{};
        request.displayName = displayName.empty() ? projectName : displayName;
        request.templateId = templateId;
        request.parentDirectory = parentDirectory;
        request.engineVersion = m_Engines.Current().engineVersion;
        request.engineRoot = PathUtils::ToUtf8(m_Engines.Current().engineRoot);

        const auto created = we::projects::ProjectLifecycle::CreateNewProject(request);
        if (!created.success) {
            result.message = created.message;
            return result;
        }

        if (!tmpl->platforms.empty()) {
            if (auto descriptor = we::projects::ProjectLifecycle::ReadDescriptor(created.weprojPath)) {
                descriptor->targetPlatforms = tmpl->platforms;
                (void)we::projects::ProjectLifecycle::WriteDescriptor(created.weprojPath, *descriptor);
            }
        }

        m_Settings.TouchRecent(PathUtils::ToUtf8(created.weprojPath));
        m_Settings.Save();

        result.success = true;
        result.weprojPath = PathUtils::ToUtf8(created.weprojPath);
        result.message = created.message;
        return result;
    }

    // File-backed templates: materialize tree, then write canonical descriptor.
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
    descriptor.contentDirectory = we::core::layout::kContent;
    descriptor.startupMap = std::string(we::core::layout::kContent) + "/Maps/Startup.scene";
    descriptor.defaultGameMode.clear();
    descriptor.modules = { projectName };
    descriptor.plugins = {};
    descriptor.targetPlatforms = tmpl->platforms.empty() ? std::vector<std::string>{ "Windows" } : tmpl->platforms;
    descriptor.settings = {
        { "startupMap", descriptor.startupMap },
        { "defaultGameMode", "" },
    };
    descriptor.createdUtc = we::projects::ProjectLifecycle::NowUtc();
    descriptor.lastOpenedUtc = descriptor.createdUtc;

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
    descriptor.lastOpenedUtc = we::projects::ProjectLifecycle::NowUtc();
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

    const auto parentDirectory = PathUtils::FromUtf8(summary->projectRoot).parent_path();
    const auto cloned = we::projects::ProjectLifecycle::CloneProject(
        sourceWeproj, newDisplayName, parentDirectory);
    if (!cloned.success) {
        result.message = cloned.message;
        return result;
    }

    m_Settings.TouchRecent(PathUtils::ToUtf8(cloned.weprojPath));
    m_Settings.Save();

    result.success = true;
    result.weprojPath = PathUtils::ToUtf8(cloned.weprojPath);
    result.message = cloned.message;
    return result;
}

ProjectOperationResult ProjectService::RenameProject(const std::filesystem::path& weprojPath, const std::string& newDisplayName) {
    ProjectOperationResult result{};
    auto summary = LoadSummary(weprojPath);
    if (!summary) {
        result.message = "Project invalid.";
        return result;
    }

    const std::string newName = we::projects::ProjectLifecycle::SanitizeProjectName(newDisplayName);
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
