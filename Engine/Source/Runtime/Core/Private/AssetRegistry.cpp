#include "Core/AssetRegistry.h"
#include "Core/Logger.h"
#include "Core/Paths.h"

#include <filesystem>
#include <fstream>

namespace we::core {
namespace {

std::vector<std::string> PathsToStrings(const std::vector<std::filesystem::path>& paths) {
    std::vector<std::string> out;
    out.reserve(paths.size());
    for (const auto& path : paths) {
        out.push_back(PathService::ToUtf8(path));
    }
    return out;
}

} // namespace

AssetRegistry& AssetRegistry::Get() {
    static AssetRegistry instance;
    return instance;
}

void AssetRegistry::RegisterTexture(std::string_view name, uint64_t view, uint64_t sampler) {
    std::unique_lock lock(m_Mutex);
    m_Textures[std::string(name)] = AssetTexture{view, sampler};
}

AssetTexture AssetRegistry::GetTexture(std::string_view name) const {
    std::shared_lock lock(m_Mutex);
    auto it = m_Textures.find(std::string(name));
    if (it != m_Textures.end()) {
        return it->second;
    }
    return {};
}

void AssetRegistry::RegisterFontPath(std::string_view name, std::string_view resolvedPath) {
    std::unique_lock lock(m_Mutex);
    m_FontPaths[std::string(name)] = std::string(resolvedPath);
}

void AssetRegistry::RegisterShaderPath(std::string_view name, std::string_view resolvedPath) {
    std::unique_lock lock(m_Mutex);
    m_ShaderPaths[std::string(name)] = std::string(resolvedPath);
}

void AssetRegistry::RegisterIconPath(std::string_view name, std::string_view resolvedPath) {
    std::unique_lock lock(m_Mutex);
    m_IconPaths[std::string(name)] = std::string(resolvedPath);
}

void AssetRegistry::RegisterIconAtlasRoot(std::string_view resolvedPath) {
    std::unique_lock lock(m_Mutex);
    m_IconAtlasRoot = std::string(resolvedPath);
}

void AssetRegistry::RegisterIconMetaPath(std::string_view resolvedPath) {
    std::unique_lock lock(m_Mutex);
    m_IconMetaPath = std::string(resolvedPath);
}

std::string AssetRegistry::GetFontPath(std::string_view name) const {
    std::shared_lock lock(m_Mutex);
    auto it = m_FontPaths.find(std::string(name));
    return it != m_FontPaths.end() ? it->second : std::string{};
}

std::string AssetRegistry::GetShaderPath(std::string_view name) const {
    std::shared_lock lock(m_Mutex);
    auto it = m_ShaderPaths.find(std::string(name));
    return it != m_ShaderPaths.end() ? it->second : std::string{};
}

std::string AssetRegistry::GetIconPath(std::string_view name) const {
    std::shared_lock lock(m_Mutex);
    auto it = m_IconPaths.find(std::string(name));
    return it != m_IconPaths.end() ? it->second : std::string{};
}

std::string AssetRegistry::GetIconAtlasRoot() const {
    std::shared_lock lock(m_Mutex);
    return m_IconAtlasRoot;
}

std::string AssetRegistry::GetIconMetaPath() const {
    std::shared_lock lock(m_Mutex);
    return m_IconMetaPath;
}

std::vector<AssetLoadResult> AssetRegistry::GetLastLoadResults() const {
    std::shared_lock lock(m_Mutex);
    return m_LastLoadResults;
}

std::string AssetRegistry::ResolveAssetPath(const std::vector<std::string>& candidates) {
    std::vector<std::filesystem::path> paths;
    paths.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        paths.push_back(PathService::FromUtf8(candidate));
    }
    if (const auto found = PathService::FindExisting(paths)) {
        return PathService::ToUtf8(*found);
    }
    return {};
}

AssetLoadResult AssetRegistry::TryLoadAsset(std::string_view name, const std::vector<std::string>& candidates) {
    // Caller must hold unique lock (LoadDefaultEditorAssets).
    AssetLoadResult result;
    result.name = std::string(name);
    result.resolvedPath = ResolveAssetPath(candidates);
    result.found = !result.resolvedPath.empty();
    m_LastLoadResults.push_back(result);
    return result;
}

bool AssetRegistry::LoadDefaultEditorAssets() {
    std::unique_lock lock(m_Mutex);
    m_LastLoadResults.clear();

    HE_INFO("[Assets] Loading default editor assets...");

    const auto& paths = PathService::Get();

    const std::vector<std::pair<std::string, std::vector<std::string>>> fonts = {
        {"Font_UI", PathsToStrings(paths.FontCandidates("Inter-Regular.wefont"))},
    };

    const std::vector<std::pair<std::string, std::string>> shaderNames = {
        {"UI", "UI_VS.spv"},
        {"AtmospherePass", "AtmospherePass_VS.spv"},
        {"VolumetricCloudsPass", "VolumetricCloudsPass_VS.spv"},
        {"CloudTemporalResolve", "CloudTemporalResolve_VS.spv"},
        {"CloudCompositePass", "CloudCompositePass_VS.spv"},
        {"FogCompositePass", "FogCompositePass_VS.spv"},
        {"EditorGrid", "EditorGrid_VS.spv"},
        {"SceneObject", "SceneObject_VS.spv"},
    };

    std::vector<std::pair<std::string, std::vector<std::string>>> shaders;
    shaders.reserve(shaderNames.size());
    for (const auto& [name, fileName] : shaderNames) {
        shaders.emplace_back(name, PathsToStrings(paths.ShaderBytecodeCandidates(fileName)));
    }

    const std::vector<std::pair<std::string, std::vector<std::string>>> icons = {
        {"Icon_Lucide", PathsToStrings(paths.IconCandidates("icons"))},
    };

    const std::vector<std::pair<std::string, std::vector<std::string>>> iconAtlases = {
        {"Icon_AtlasRoot", PathsToStrings(paths.IconCandidates("Atlas"))},
    };

    const std::vector<std::pair<std::string, std::vector<std::string>>> iconMeta = {
        {"Icon_Meta", PathsToStrings(paths.IconCandidates(std::filesystem::path("Atlas") / "icons.weiconmeta"))},
    };

    bool allRequiredFound = true;

    for (const auto& [name, candidatePaths] : fonts) {
        auto result = TryLoadAsset(name, candidatePaths);
        if (result.found) {
            m_FontPaths[name] = result.resolvedPath;
            HE_INFO("[Assets]   Font '" + name + "' -> " + result.resolvedPath);
        } else {
            HE_ERROR("[Assets]   MISSING font '" + name + "'");
            allRequiredFound = false;
        }
    }

    for (const auto& [name, candidatePaths] : shaders) {
        auto result = TryLoadAsset(name, candidatePaths);
        if (result.found) {
            m_ShaderPaths[name] = result.resolvedPath;
            HE_INFO("[Assets]   Shader '" + name + "' -> " + result.resolvedPath);
        } else {
            const bool required = (name == "UI");
            if (required) {
                HE_ERROR("[Assets]   MISSING required shader '" + name + "'");
                allRequiredFound = false;
            } else {
                HE_INFO("[Assets]   Optional shader '" + name + "' not found (may compile later)");
            }
        }
    }

    for (const auto& [name, candidatePaths] : icons) {
        auto result = TryLoadAsset(name, candidatePaths);
        if (result.found) {
            m_IconPaths[name] = result.resolvedPath;
            HE_INFO("[Assets]   Icon source '" + name + "' -> " + result.resolvedPath);
        } else {
            HE_INFO("[Assets]   Optional icon source '" + name + "' not found (offline import only)");
        }
    }

    for (const auto& [name, candidatePaths] : iconAtlases) {
        auto result = TryLoadAsset(name, candidatePaths);
        if (result.found) {
            m_IconAtlasRoot = result.resolvedPath;
            HE_INFO("[Assets]   Icon atlas root '" + name + "' -> " + result.resolvedPath);
        } else {
            HE_ERROR("[Assets]   MISSING icon atlas root '" + name + "'");
            allRequiredFound = false;
        }
    }

    for (const auto& [name, candidatePaths] : iconMeta) {
        auto result = TryLoadAsset(name, candidatePaths);
        if (result.found) {
            m_IconMetaPath = result.resolvedPath;
            HE_INFO("[Assets]   Icon meta '" + name + "' -> " + result.resolvedPath);
        } else {
            HE_ERROR("[Assets]   MISSING icon meta '" + name + "'");
            allRequiredFound = false;
        }
    }

    HE_INFO("[Assets] Theme provider initialized (GraphiteDarkTheme tokens)");
    HE_INFO("[Assets] Default asset load " + std::string(allRequiredFound ? "succeeded" : "FAILED"));
    return allRequiredFound;
}

void AssetRegistry::Clear() {
    std::unique_lock lock(m_Mutex);
    m_Textures.clear();
    m_FontPaths.clear();
    m_ShaderPaths.clear();
    m_IconPaths.clear();
    m_IconAtlasRoot.clear();
    m_IconMetaPath.clear();
    m_LastLoadResults.clear();
}

} // namespace we::core
