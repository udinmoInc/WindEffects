#include "Core/Paths.h"
#include "Core/BuildPaths.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>
#endif

namespace we::core {
namespace {

std::filesystem::path WeakCanonical(const std::filesystem::path& path) {
    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(path, ec);
    if (ec) {
        return path.lexically_normal();
    }
    return canonical;
}

std::optional<std::filesystem::path> TryFindMarker(const std::filesystem::path& start) {
    if (start.empty()) {
        return std::nullopt;
    }
    std::error_code ec;
    auto current = std::filesystem::absolute(start, ec);
    if (ec) {
        current = start;
    }
    current = WeakCanonical(current);

    for (int i = 0; i < 16; ++i) {
        if (std::filesystem::exists(current / layout::kEngineMarker, ec)) {
            return current;
        }
        if (!current.has_parent_path() || current.parent_path() == current) {
            break;
        }
        current = current.parent_path();
    }
    return std::nullopt;
}

std::filesystem::path ResolveUserDataRoot() {
#if defined(_WIN32)
    wchar_t* localApp = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localApp)) && localApp) {
        std::filesystem::path base = localApp;
        CoTaskMemFree(localApp);
        return base / layout::kUserProduct;
    }
    if (const char* local = std::getenv("LOCALAPPDATA"); local && local[0] != '\0') {
        return std::filesystem::path(local) / layout::kUserProduct;
    }
    return std::filesystem::path(".") / layout::kUserProduct;
#else
    if (const char* config = std::getenv("XDG_CONFIG_HOME"); config && config[0] != '\0') {
        return std::filesystem::path(config) / "windeffects";
    }
    const char* home = std::getenv("HOME");
    return std::filesystem::path(home && home[0] != '\0' ? home : ".") / ".config" / "windeffects";
#endif
}

std::filesystem::path ResolveDefaultProjectsRoot() {
#if defined(_WIN32)
    if (const char* profile = std::getenv("USERPROFILE"); profile && profile[0] != '\0') {
        return std::filesystem::path(profile) / layout::kUserProduct / layout::kProjects;
    }
    wchar_t* docs = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs)) && docs) {
        std::filesystem::path base = docs;
        CoTaskMemFree(docs);
        return base / layout::kUserProduct / layout::kProjects;
    }
    return std::filesystem::path(".") / layout::kUserProduct / layout::kProjects;
#else
    const char* home = std::getenv("HOME");
    return std::filesystem::path(home && home[0] != '\0' ? home : ".") / layout::kUserProduct / layout::kProjects;
#endif
}

std::optional<std::filesystem::path> TryFindEngineSource(const std::filesystem::path& start) {
    if (start.empty()) {
        return std::nullopt;
    }
    std::error_code ec;
    auto current = std::filesystem::absolute(start, ec);
    if (ec) {
        current = start;
    }
    current = WeakCanonical(current);

    for (int i = 0; i < 16; ++i) {
        if (std::filesystem::exists(current / layout::kEngine / layout::kSource, ec)) {
            return current;
        }
        if (!current.has_parent_path() || current.parent_path() == current) {
            break;
        }
        current = current.parent_path();
    }
    return std::nullopt;
}

} // namespace

PathService& PathService::Get() {
    static PathService instance;
    return instance;
}

void PathService::EnsureBootstrapped() const {
    if (!m_Configured) {
        BootstrapUnlocked();
    }
}

void PathService::BootstrapUnlocked() const {
    Configuration config = m_Config;
    if (config.executableDirectory.empty()) {
        config.executableDirectory = GetExecutableDirectory();
        if (config.executableDirectory.empty()) {
            std::error_code ec;
            config.executableDirectory = std::filesystem::current_path(ec);
        }
    }
    config.executableDirectory = Absolute(config.executableDirectory);

    if (config.engineRoot.empty()) {
        if (const auto found = FindEngineRoot(config.executableDirectory)) {
            config.engineRoot = *found;
        } else {
            config.engineRoot = config.executableDirectory;
            m_WarnedMissingMarker = true;
        }
    }
    config.engineRoot = Absolute(config.engineRoot);

    if (config.userDataRoot.empty()) {
        config.userDataRoot = ResolveUserDataRoot();
    }

    if (config.logsRoot.empty()) {
        if (!config.savedRoot.empty()) {
            config.logsRoot = config.savedRoot / layout::kLogs;
        } else {
            config.logsRoot = config.executableDirectory / layout::kSaved / layout::kLogs;
        }
    }

    m_Config = std::move(config);
    m_Configured = true;
}

void PathService::Configure(const Configuration& config) {
    if (!config.executableDirectory.empty()) {
        m_Config.executableDirectory = Absolute(config.executableDirectory);
    }
    if (!config.engineRoot.empty()) {
        m_Config.engineRoot = Absolute(config.engineRoot);
    }
    if (!config.projectRoot.empty()) {
        m_Config.projectRoot = Absolute(config.projectRoot);
    }
    if (!config.contentRoot.empty()) {
        m_Config.contentRoot = Absolute(config.contentRoot);
    }
    if (!config.savedRoot.empty()) {
        m_Config.savedRoot = Absolute(config.savedRoot);
    }
    if (!config.intermediateRoot.empty()) {
        m_Config.intermediateRoot = Absolute(config.intermediateRoot);
    }
    if (!config.configRoot.empty()) {
        m_Config.configRoot = Absolute(config.configRoot);
    }
    if (!config.pluginsRoot.empty()) {
        m_Config.pluginsRoot = Absolute(config.pluginsRoot);
    }
    if (!config.cookedRoot.empty()) {
        m_Config.cookedRoot = Absolute(config.cookedRoot);
    }
    if (!config.logsRoot.empty()) {
        m_Config.logsRoot = Absolute(config.logsRoot);
    }
    if (!config.cacheRoot.empty()) {
        m_Config.cacheRoot = Absolute(config.cacheRoot);
    }
    if (!config.userDataRoot.empty()) {
        m_Config.userDataRoot = Absolute(config.userDataRoot);
    }

    // Re-derive defaults that depend on newly set roots.
    if (m_Config.logsRoot.empty()) {
        if (!m_Config.savedRoot.empty()) {
            m_Config.logsRoot = m_Config.savedRoot / layout::kLogs;
        } else if (!m_Config.executableDirectory.empty()) {
            m_Config.logsRoot = m_Config.executableDirectory / layout::kSaved / layout::kLogs;
        }
    }

    if (m_Config.executableDirectory.empty() || m_Config.engineRoot.empty()) {
        BootstrapUnlocked();
    } else {
        m_Configured = true;
    }
}

void PathService::ClearProjectRoots() {
    m_Config.projectRoot.clear();
    m_Config.contentRoot.clear();
    m_Config.savedRoot.clear();
    m_Config.intermediateRoot.clear();
    m_Config.configRoot.clear();
    m_Config.pluginsRoot.clear();
    m_Config.cookedRoot.clear();
    m_Config.cacheRoot.clear();
    // Keep logs under runtime saved (exe) after unload.
    if (!m_Config.executableDirectory.empty()) {
        m_Config.logsRoot = m_Config.executableDirectory / layout::kSaved / layout::kLogs;
    }
}

void PathService::Shutdown() {
    m_Configured = false;
    m_WarnedMissingMarker = false;
    m_Config = {};
}

std::filesystem::path PathService::ExecutableDirectory() const {
    EnsureBootstrapped();
    return m_Config.executableDirectory;
}

std::filesystem::path PathService::EngineRoot() const {
    EnsureBootstrapped();
    return m_Config.engineRoot;
}

std::filesystem::path PathService::EngineContentRoot() const {
    return Join(EngineRoot(), layout::kEngine, layout::kContent);
}

std::filesystem::path PathService::EngineConfigRoot() const {
    return Join(EngineRoot(), layout::kEngine, layout::kConfig);
}

std::filesystem::path PathService::EngineShadersRoot() const {
    return Join(EngineRoot(), layout::kEngine, layout::kShaders);
}

std::filesystem::path PathService::EngineShaderBytecodesRoot() const {
    return Join(EngineShadersRoot(), layout::kBytecodes);
}

std::filesystem::path PathService::EngineBinariesRoot() const {
    return Join(EngineRoot(), layout::kEngine, layout::kBinaries);
}

std::filesystem::path PathService::EngineResourcesRoot() const {
    return Join(EngineRoot(), layout::kEngine, layout::kResources);
}

std::filesystem::path PathService::EngineFontsRoot() const {
    return Join(EngineContentRoot(), layout::kFonts);
}

std::filesystem::path PathService::EngineIconsRoot() const {
    return Join(EngineContentRoot(), layout::kIcons);
}

std::filesystem::path PathService::TemplatesRoot() const {
    return Join(Join(EngineRoot(), layout::kEngine), layout::kTemplates) / layout::kProjects;
}

std::filesystem::path PathService::StagedAssetsRoot() const {
    // Staged product layout places Assets next to the executable.
    return Join(ExecutableDirectory(), layout::kAssets);
}

std::filesystem::path PathService::ThirdPartyRoot() const {
    return Join(ExecutableDirectory(), layout::kThirdParty);
}

bool PathService::HasProject() const {
    EnsureBootstrapped();
    return !m_Config.projectRoot.empty();
}

std::filesystem::path PathService::ProjectRoot() const {
    EnsureBootstrapped();
    return m_Config.projectRoot;
}

std::filesystem::path PathService::ContentRoot() const {
    EnsureBootstrapped();
    if (!m_Config.contentRoot.empty()) {
        return m_Config.contentRoot;
    }
    if (!m_Config.projectRoot.empty()) {
        return m_Config.projectRoot / layout::kContent;
    }
    return {};
}

std::filesystem::path PathService::PluginsRoot() const {
    EnsureBootstrapped();
    if (!m_Config.pluginsRoot.empty()) {
        return m_Config.pluginsRoot;
    }
    if (!m_Config.projectRoot.empty()) {
        return m_Config.projectRoot / layout::kPlugins;
    }
    return {};
}

std::filesystem::path PathService::ConfigRoot() const {
    EnsureBootstrapped();
    if (!m_Config.configRoot.empty()) {
        return m_Config.configRoot;
    }
    if (!m_Config.projectRoot.empty()) {
        return m_Config.projectRoot / layout::kConfig;
    }
    return EngineConfigRoot();
}

std::filesystem::path PathService::SavedRoot() const {
    EnsureBootstrapped();
    if (!m_Config.savedRoot.empty()) {
        return m_Config.savedRoot;
    }
    if (!m_Config.projectRoot.empty()) {
        return m_Config.projectRoot / layout::kSaved;
    }
    return RuntimeSavedRoot();
}

std::filesystem::path PathService::IntermediateRoot() const {
    EnsureBootstrapped();
    if (!m_Config.intermediateRoot.empty()) {
        return m_Config.intermediateRoot;
    }
    if (!m_Config.projectRoot.empty()) {
        return m_Config.projectRoot / layout::kIntermediate;
    }
    return ExecutableDirectory() / layout::kIntermediate;
}

std::filesystem::path PathService::CookedRoot() const {
    EnsureBootstrapped();
    if (!m_Config.cookedRoot.empty()) {
        return m_Config.cookedRoot;
    }
    return IntermediateRoot() / layout::kCooked;
}

std::filesystem::path PathService::CookedRootForPlatform(std::string_view platformName) const {
    const std::string name = platformName.empty() ? "Custom" : std::string(platformName);
    return CookedRoot() / name;
}

std::filesystem::path PathService::AssetPipelineRoot() const {
    return IntermediateRoot() / layout::kAssetPipeline;
}

std::filesystem::path PathService::ImportedRoot() const {
    return AssetPipelineRoot() / layout::kImported;
}

std::filesystem::path PathService::ProjectCacheRoot() const {
    EnsureBootstrapped();
    if (!m_Config.cacheRoot.empty()) {
        return m_Config.cacheRoot;
    }
    return SavedRoot() / layout::kCache;
}

std::filesystem::path PathService::ThumbnailCacheRoot() const {
    return ProjectCacheRoot() / layout::kThumbnails;
}

std::filesystem::path PathService::RuntimeSavedRoot() const {
    return Join(ExecutableDirectory(), layout::kSaved);
}

std::filesystem::path PathService::LogsRoot() const {
    EnsureBootstrapped();
    if (!m_Config.logsRoot.empty()) {
        return m_Config.logsRoot;
    }
    return SavedRoot() / layout::kLogs;
}

std::filesystem::path PathService::CrashesRoot() const {
    return SavedRoot() / layout::kCrashes;
}

std::filesystem::path PathService::CrashLatestRoot() const {
    return CrashesRoot() / "Latest";
}

std::filesystem::path PathService::UserDataRoot() const {
    EnsureBootstrapped();
    if (!m_Config.userDataRoot.empty()) {
        return m_Config.userDataRoot;
    }
    return ResolveUserDataRoot();
}

std::filesystem::path PathService::UserCacheRoot() const {
    return UserDataRoot() / layout::kCache;
}

std::filesystem::path PathService::UserLogsRoot() const {
    return UserDataRoot() / layout::kLogs;
}

std::filesystem::path PathService::DefaultProjectsRoot() const {
    return ResolveDefaultProjectsRoot();
}

std::filesystem::path PathService::Join(const std::filesystem::path& a, const std::filesystem::path& b) {
    return Normalize(a / b);
}

std::filesystem::path PathService::Join(
    const std::filesystem::path& a,
    const std::filesystem::path& b,
    const std::filesystem::path& c) {
    return Normalize(a / b / c);
}

std::filesystem::path PathService::Normalize(const std::filesystem::path& path) {
    return path.lexically_normal();
}

std::filesystem::path PathService::Absolute(const std::filesystem::path& path) {
    if (path.empty()) {
        return {};
    }
    std::error_code ec;
    auto absolute = std::filesystem::absolute(path, ec);
    if (ec) {
        return Normalize(path);
    }
    return WeakCanonical(absolute);
}

std::string PathService::ToGeneric(const std::filesystem::path& path) {
    return path.generic_string();
}

std::filesystem::path PathService::FromUtf8(std::string_view utf8) {
#if defined(_WIN32)
    if (utf8.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(
        CP_UTF8,
        0,
        utf8.data(),
        static_cast<int>(utf8.size()),
        nullptr,
        0);
    if (size <= 0) {
        return std::filesystem::path(std::string(utf8));
    }
    std::wstring wide(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        0,
        utf8.data(),
        static_cast<int>(utf8.size()),
        wide.data(),
        size);
    return std::filesystem::path(wide);
#else
    return std::filesystem::u8path(std::string(utf8));
#endif
}

std::string PathService::ToUtf8(const std::filesystem::path& path) {
#if defined(_WIN32)
    const std::wstring wide = path.native();
    if (wide.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        nullptr,
        0,
        nullptr,
        nullptr);
    if (size <= 0) {
        return {};
    }
    std::string out(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        out.data(),
        size,
        nullptr,
        nullptr);
    return out;
#else
    return path.string();
#endif
}

std::filesystem::path PathService::ResolveRelative(
    const std::filesystem::path& root,
    const std::filesystem::path& relative) {
    if (relative.empty()) {
        return Absolute(root);
    }
    if (relative.is_absolute()) {
        return Absolute(relative);
    }
    return Absolute(root / relative);
}

std::optional<std::filesystem::path> PathService::FindExisting(
    std::span<const std::filesystem::path> candidates) {
    std::error_code ec;
    for (const auto& candidate : candidates) {
        if (candidate.empty()) {
            continue;
        }
        if (std::filesystem::exists(candidate, ec)) {
            return Absolute(candidate);
        }
    }
    return std::nullopt;
}

std::vector<std::filesystem::path> PathService::ResourceCandidates(
    const std::filesystem::path& relativeUnderRoots) const {
    std::vector<std::filesystem::path> out;
    const auto add = [&](const std::filesystem::path& root) {
        if (root.empty()) {
            return;
        }
        out.push_back(root / relativeUnderRoots);
    };

    add(EngineContentRoot());
    add(Join(EngineRoot(), layout::kEngine));
    add(StagedAssetsRoot());
    add(ExecutableDirectory());
    add(Join(ExecutableDirectory(), layout::kEngine, layout::kContent));
    return out;
}

std::vector<std::filesystem::path> PathService::ShaderBytecodeCandidates(
    const std::filesystem::path& fileName) const {
    std::vector<std::filesystem::path> out;
    out.push_back(EngineShaderBytecodesRoot() / fileName);
    out.push_back(Join(ExecutableDirectory(), layout::kEngine, layout::kShaders) / layout::kBytecodes / fileName);
    out.push_back(StagedAssetsRoot() / layout::kShaders / fileName);
    out.push_back(ExecutableDirectory() / layout::kShaders / fileName);
    return out;
}

std::vector<std::filesystem::path> PathService::FontCandidates(
    const std::filesystem::path& fileName) const {
    std::vector<std::filesystem::path> out;
    out.push_back(EngineFontsRoot() / fileName);
    out.push_back(StagedAssetsRoot() / layout::kFonts / fileName);
    out.push_back(ExecutableDirectory() / layout::kFonts / fileName);
    out.push_back(Join(EngineConfigRoot(), layout::kFonts) / fileName);
    return out;
}

std::vector<std::filesystem::path> PathService::IconCandidates(
    const std::filesystem::path& relativeUnderIcons) const {
    std::vector<std::filesystem::path> out;
    out.push_back(EngineIconsRoot() / relativeUnderIcons);
    out.push_back(StagedAssetsRoot() / layout::kIcons / relativeUnderIcons);
    out.push_back(ExecutableDirectory() / layout::kIcons / relativeUnderIcons);
    out.push_back(StagedAssetsRoot() / layout::kEditor / relativeUnderIcons);
    return out;
}

PathValidationResult PathService::ValidateDirectory(
    const std::filesystem::path& path,
    bool requireWritable) {
    PathValidationResult result;
    result.path = Absolute(path);
    if (result.path.empty()) {
        result.message = "Path is empty.";
        return result;
    }

    std::error_code ec;
    result.exists = std::filesystem::exists(result.path, ec);
    if (ec) {
        result.message = "Unable to query path: " + ec.message();
        return result;
    }
    if (!result.exists) {
        result.message = "Directory does not exist: " + ToGeneric(result.path);
        return result;
    }

    result.isDirectory = std::filesystem::is_directory(result.path, ec);
    if (ec || !result.isDirectory) {
        result.message = "Path is not a directory: " + ToGeneric(result.path);
        return result;
    }

    // Accessibility probe: attempt directory_iterator.
    auto it = std::filesystem::directory_iterator(
        result.path,
        std::filesystem::directory_options::skip_permission_denied,
        ec);
    (void)it;
    result.accessible = !ec;
    if (!result.accessible) {
        result.message = "Directory is not accessible: " + ToGeneric(result.path) + " (" + ec.message() + ")";
        return result;
    }

    if (requireWritable) {
        const auto probe = result.path / ".we_path_write_probe";
        std::ofstream file(probe, std::ios::out | std::ios::trunc);
        if (!file.is_open()) {
            result.message = "Directory is not writable: " + ToGeneric(result.path);
            return result;
        }
        file.close();
        std::filesystem::remove(probe, ec);
    }

    result.ok = true;
    result.message = "OK";
    return result;
}

PathValidationResult PathService::EnsureDirectory(const std::filesystem::path& path) {
    PathValidationResult result;
    result.path = Absolute(path);
    if (result.path.empty()) {
        result.message = "Path is empty.";
        return result;
    }

    std::error_code ec;
    std::filesystem::create_directories(result.path, ec);
    if (ec) {
        result.message = "Failed to create directory " + ToGeneric(result.path) + ": " + ec.message();
        return result;
    }
    return ValidateDirectory(result.path, false);
}

std::optional<std::filesystem::path> PathService::FindEngineRoot(
    const std::filesystem::path& startDirectory) {
    if (const char* envRoot = std::getenv("WE_ENGINE_ROOT"); envRoot && envRoot[0] != '\0') {
        if (auto found = TryFindMarker(FromUtf8(envRoot))) {
            return found;
        }
        // Explicit override may point directly at the install root without walking.
        std::error_code ec;
        const auto direct = Absolute(FromUtf8(envRoot));
        if (std::filesystem::exists(direct / layout::kEngineMarker, ec)) {
            return direct;
        }
        if (std::filesystem::is_directory(direct, ec)) {
            return direct;
        }
    }
    if (const char* envRoot = std::getenv("WE_PROJECT_ROOT"); envRoot && envRoot[0] != '\0') {
        if (auto found = TryFindMarker(FromUtf8(envRoot))) {
            return found;
        }
    }
    if (auto found = TryFindMarker(startDirectory)) {
        return found;
    }
    std::error_code ec;
    return TryFindMarker(std::filesystem::current_path(ec));
}

std::optional<std::filesystem::path> PathService::FindRepositoryRoot(
    const std::filesystem::path& startDirectory) {
    // Prefer the nearest Engine/Source tree before WindEffects.engine. Temp/test trees
    // often live under the real checkout (e.g. Build/Temp/...), and a marker walk would
    // otherwise escape to the developer install root.
    if (auto source = TryFindEngineSource(startDirectory)) {
        return source;
    }
    if (auto marker = TryFindMarker(startDirectory)) {
        return marker;
    }

    if (const char* envRoot = std::getenv("WE_ENGINE_ROOT"); envRoot && envRoot[0] != '\0') {
        if (auto found = TryFindEngineSource(FromUtf8(envRoot))) {
            return found;
        }
        if (auto found = TryFindMarker(FromUtf8(envRoot))) {
            return found;
        }
    }
    if (const char* envRoot = std::getenv("WE_PROJECT_ROOT"); envRoot && envRoot[0] != '\0') {
        if (auto found = TryFindEngineSource(FromUtf8(envRoot))) {
            return found;
        }
        if (auto found = TryFindMarker(FromUtf8(envRoot))) {
            return found;
        }
    }

    std::error_code ec;
    const auto cwd = std::filesystem::current_path(ec);
    if (auto source = TryFindEngineSource(cwd)) {
        return source;
    }
    return TryFindMarker(cwd);
}

LayoutValidationReport PathService::ValidateLayout(bool requireProject) const {
    EnsureBootstrapped();
    LayoutValidationReport report;

    const auto add = [&](std::string name, const std::filesystem::path& path, bool required, bool writable = false) {
        LayoutCheck check;
        check.name = std::move(name);
        check.required = required;
        if (path.empty()) {
            check.result.message = "Path is empty.";
            check.result.path.clear();
        } else {
            check.result = ValidateDirectory(path, writable);
        }
        report.checks.push_back(std::move(check));
    };

    add("EngineRoot", EngineRoot(), true);
    add("EngineContent", EngineContentRoot(), true);
    add("EngineConfig", EngineConfigRoot(), true);
    add("ExecutableDirectory", ExecutableDirectory(), true);
    add("Build", Join(EngineRoot(), layout::kBuild), false);
    add("Cache", UserCacheRoot(), false);

    if (requireProject || HasProject()) {
        add("ProjectRoot", ProjectRoot(), true);
        add("Content", ContentRoot(), true);
        add("Config", ConfigRoot(), true);
        add("Plugins", PluginsRoot(), false);
        add("Saved", SavedRoot(), true, true);
        add("Intermediate", IntermediateRoot(), false);
        add("Cooked", CookedRoot(), false);
        add("ProjectCache", ProjectCacheRoot(), false);
        add("Logs", LogsRoot(), false);
    } else {
        add("RuntimeSaved", RuntimeSavedRoot(), false);
        add("Logs", LogsRoot(), false);
    }

    report.ok = true;
    std::ostringstream summary;
    for (const auto& check : report.checks) {
        if (check.required && !check.result.ok) {
            report.ok = false;
            summary << check.name << ": " << check.result.message << "; ";
        }
    }
    if (report.ok) {
        report.summary = "Layout validation passed.";
    } else {
        report.summary = summary.str();
        if (report.summary.empty()) {
            report.summary = "Layout validation failed.";
        }
    }
    return report;
}

std::optional<std::filesystem::path> PathService::FindIgniteBTExecutable() const {
    const auto repo = FindRepositoryRoot(ExecutableDirectory());
    if (!repo) {
        return std::nullopt;
    }
#if defined(_WIN32)
    const char* binaryName = "IgniteBT.exe";
#else
    const char* binaryName = "IgniteBT";
#endif
    const char* configurations[] = { "Debug", "Release", "Development", "Shipping" };
    std::vector<std::filesystem::path> candidates;
    for (const char* config : configurations) {
        candidates.push_back(
            *repo / layout::kBuild / layout::kIntermediate / "IgniteBT" / config / "net8.0" / binaryName);
    }
    return FindExisting(candidates);
}

std::filesystem::path PathService::ResolveSiblingExecutable(std::string_view fileName) const {
    return Join(ExecutableDirectory(), std::filesystem::path(fileName));
}

} // namespace we::core
