#pragma once

#include "Core/Export.h"

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::core {

/// Canonical layout directory names. Prefer these over string literals.
namespace layout {
inline constexpr const char* kEngine = "Engine";
inline constexpr const char* kContent = "Content";
inline constexpr const char* kConfig = "Config";
inline constexpr const char* kShaders = "Shaders";
inline constexpr const char* kBytecodes = "Bytecodes";
inline constexpr const char* kBinaries = "Binaries";
inline constexpr const char* kResources = "Resources";
inline constexpr const char* kPlugins = "Plugins";
inline constexpr const char* kSaved = "Saved";
inline constexpr const char* kIntermediate = "Intermediate";
inline constexpr const char* kBuild = "Build";
inline constexpr const char* kAssets = "Assets";
inline constexpr const char* kCooked = "Cooked";
inline constexpr const char* kCache = "Cache";
inline constexpr const char* kLogs = "Logs";
inline constexpr const char* kCrashes = "Crashes";
inline constexpr const char* kThumbnails = "Thumbnails";
inline constexpr const char* kTemp = "Temp";
inline constexpr const char* kPackages = "Packages";
inline constexpr const char* kSource = "Source";
inline constexpr const char* kThirdParty = "ThirdParty";
inline constexpr const char* kFonts = "Fonts";
inline constexpr const char* kIcons = "Icons";
inline constexpr const char* kEditor = "Editor";
inline constexpr const char* kTemplates = "Templates";
inline constexpr const char* kProjects = "Projects";
inline constexpr const char* kAssetPipeline = "AssetPipeline";
inline constexpr const char* kImported = "Imported";
inline constexpr const char* kSessions = "Sessions";
inline constexpr const char* kEngineMarker = "WindEffects.engine";
inline constexpr const char* kUserProduct = "WindEffects";
} // namespace layout

/// Result of validating a filesystem directory for use as an engine/project root.
struct PathValidationResult {
    bool ok = false;
    bool exists = false;
    bool isDirectory = false;
    bool accessible = false;
    std::string message;
    std::filesystem::path path;
};

/// Named layout slot checked by ValidateLayout().
struct LayoutCheck {
    std::string name;
    PathValidationResult result;
    bool required = true;
};

/// Aggregate validation of engine / project layout roots.
struct LayoutValidationReport {
    bool ok = false;
    std::vector<LayoutCheck> checks;
    std::string summary;

    [[nodiscard]] const LayoutCheck* Find(std::string_view name) const {
        for (const auto& check : checks) {
            if (check.name == name) {
                return &check;
            }
        }
        return nullptr;
    }
};

/// Central path resolution service. All subsystems should obtain disk locations here
/// rather than embedding string literals or walking "../" candidates.
///
/// Bootstrap order:
/// 1. Optional Configure() from EngineContext / host
/// 2. Env WE_ENGINE_ROOT / WE_PROJECT_ROOT
/// 3. Walk parents for WindEffects.engine from executable directory
/// 4. Fall back to executable directory with diagnostics (never silent hardcodes)
class CORE_API PathService {
public:
    struct Configuration {
        std::filesystem::path executableDirectory;
        std::filesystem::path engineRoot;
        std::filesystem::path projectRoot;
        std::filesystem::path contentRoot;
        std::filesystem::path savedRoot;
        std::filesystem::path intermediateRoot;
        std::filesystem::path configRoot;
        std::filesystem::path pluginsRoot;
        std::filesystem::path cookedRoot;
        std::filesystem::path logsRoot;
        std::filesystem::path cacheRoot;
        std::filesystem::path userDataRoot;
    };

    static PathService& Get();

    /// Apply host-provided roots. Empty fields keep previously resolved values.
    void Configure(const Configuration& config);

    /// Clear project-scoped roots (after ProjectContext::Unload). Engine roots remain.
    void ClearProjectRoots();

    void Shutdown();

    [[nodiscard]] bool IsConfigured() const { return m_Configured; }

    // --- Absolute roots ---
    [[nodiscard]] std::filesystem::path ExecutableDirectory() const;
    [[nodiscard]] std::filesystem::path EngineRoot() const;
    [[nodiscard]] std::filesystem::path EngineContentRoot() const;
    [[nodiscard]] std::filesystem::path EngineConfigRoot() const;
    [[nodiscard]] std::filesystem::path EngineShadersRoot() const;
    [[nodiscard]] std::filesystem::path EngineShaderBytecodesRoot() const;
    [[nodiscard]] std::filesystem::path EngineBinariesRoot() const;
    [[nodiscard]] std::filesystem::path EngineResourcesRoot() const;
    [[nodiscard]] std::filesystem::path EngineFontsRoot() const;
    [[nodiscard]] std::filesystem::path EngineIconsRoot() const;
    [[nodiscard]] std::filesystem::path TemplatesRoot() const;
    [[nodiscard]] std::filesystem::path StagedAssetsRoot() const;
    [[nodiscard]] std::filesystem::path ThirdPartyRoot() const;

    [[nodiscard]] bool HasProject() const;
    [[nodiscard]] std::filesystem::path ProjectRoot() const;
    [[nodiscard]] std::filesystem::path ContentRoot() const;
    [[nodiscard]] std::filesystem::path PluginsRoot() const;
    [[nodiscard]] std::filesystem::path ConfigRoot() const;
    [[nodiscard]] std::filesystem::path SavedRoot() const;
    [[nodiscard]] std::filesystem::path IntermediateRoot() const;
    [[nodiscard]] std::filesystem::path CookedRoot() const;
    [[nodiscard]] std::filesystem::path CookedRootForPlatform(std::string_view platformName) const;
    [[nodiscard]] std::filesystem::path AssetPipelineRoot() const;
    [[nodiscard]] std::filesystem::path ImportedRoot() const;
    [[nodiscard]] std::filesystem::path ProjectCacheRoot() const;
    [[nodiscard]] std::filesystem::path ThumbnailCacheRoot() const;

    /// Runtime Saved under the staged product (exe directory) when no project is loaded.
    [[nodiscard]] std::filesystem::path RuntimeSavedRoot() const;
    [[nodiscard]] std::filesystem::path LogsRoot() const;
    [[nodiscard]] std::filesystem::path CrashesRoot() const;
    [[nodiscard]] std::filesystem::path CrashLatestRoot() const;

    /// Per-user product data (AppData / XDG), not tied to engine or project install.
    [[nodiscard]] std::filesystem::path UserDataRoot() const;
    [[nodiscard]] std::filesystem::path UserCacheRoot() const;
    [[nodiscard]] std::filesystem::path UserLogsRoot() const;
    [[nodiscard]] std::filesystem::path DefaultProjectsRoot() const;

    // --- Path algebra (platform-independent separators internally via generic_string) ---
    [[nodiscard]] static std::filesystem::path Join(const std::filesystem::path& a, const std::filesystem::path& b);
    [[nodiscard]] static std::filesystem::path Join(
        const std::filesystem::path& a,
        const std::filesystem::path& b,
        const std::filesystem::path& c);
    [[nodiscard]] static std::filesystem::path Normalize(const std::filesystem::path& path);
    [[nodiscard]] static std::filesystem::path Absolute(const std::filesystem::path& path);
    [[nodiscard]] static std::string ToGeneric(const std::filesystem::path& path);
    [[nodiscard]] static std::filesystem::path FromUtf8(std::string_view utf8);
    [[nodiscard]] static std::string ToUtf8(const std::filesystem::path& path);

    /// Resolve a path relative to a root (rejects empty relative).
    [[nodiscard]] static std::filesystem::path ResolveRelative(
        const std::filesystem::path& root,
        const std::filesystem::path& relative);

    /// First existing path among candidates, or empty.
    [[nodiscard]] static std::optional<std::filesystem::path> FindExisting(
        std::span<const std::filesystem::path> candidates);

    /// Build engine/staged resource candidates for a relative path under known roots
    /// (Engine/Content, Engine/..., staged Assets, exe dir). No "../" walks.
    [[nodiscard]] std::vector<std::filesystem::path> ResourceCandidates(
        const std::filesystem::path& relativeUnderRoots) const;

    /// Candidates for shader bytecode file name (e.g. "UI_VS.spv").
    [[nodiscard]] std::vector<std::filesystem::path> ShaderBytecodeCandidates(
        const std::filesystem::path& fileName) const;

    /// Candidates for a font file under engine content / staged Assets / Fonts.
    [[nodiscard]] std::vector<std::filesystem::path> FontCandidates(
        const std::filesystem::path& fileName) const;

    /// Candidates for icon / atlas resources.
    [[nodiscard]] std::vector<std::filesystem::path> IconCandidates(
        const std::filesystem::path& relativeUnderIcons) const;

    [[nodiscard]] static PathValidationResult ValidateDirectory(
        const std::filesystem::path& path,
        bool requireWritable = false);

    /// Ensure directory exists; returns validation with diagnostics on failure.
    [[nodiscard]] static PathValidationResult EnsureDirectory(const std::filesystem::path& path);

    /// Validate configured engine/project layout roots with precise diagnostics.
    /// Does not silently probe alternate locations — reports each slot independently.
    [[nodiscard]] LayoutValidationReport ValidateLayout(bool requireProject = false) const;

    /// Discover engine/install root by walking parents for WindEffects.engine.
    /// Honors WE_ENGINE_ROOT then WE_PROJECT_ROOT env overrides.
    [[nodiscard]] static std::optional<std::filesystem::path> FindEngineRoot(
        const std::filesystem::path& startDirectory);

    /// Repository / checkout root: WindEffects.engine, else directory containing Engine/Source.
    /// Used by build-tool launchers that need the source tree, not only the marker.
    [[nodiscard]] static std::optional<std::filesystem::path> FindRepositoryRoot(
        const std::filesystem::path& startDirectory);

    /// Program binary next to the executable (e.g. WECrashReporter.exe).
    [[nodiscard]] std::filesystem::path ResolveSiblingExecutable(std::string_view fileName) const;

    /// IgniteBT binary under Build/Intermediate (build-tool bridge only).
    [[nodiscard]] std::optional<std::filesystem::path> FindIgniteBTExecutable() const;

private:
    PathService() = default;

    void EnsureBootstrapped() const;
    void BootstrapUnlocked() const;

    mutable bool m_Configured = false;
    mutable bool m_WarnedMissingMarker = false;
    mutable Configuration m_Config{};
};

} // namespace we::core
