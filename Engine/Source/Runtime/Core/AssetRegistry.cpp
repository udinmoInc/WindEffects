#include "AssetRegistry.h"
#include "Core/Logger.h"

#include <fstream>
#include <filesystem>

#if WE_HAS_VULKAN
#include <volk.h>
#endif

namespace we::core {

AssetRegistry& AssetRegistry::Get() {
    static AssetRegistry instance;
    return instance;
}

#if WE_HAS_VULKAN
void AssetRegistry::RegisterTexture(std::string_view name, VkImageView view, VkSampler sampler) {
    m_Textures[std::string(name)] = {view, sampler};
}
#else
void AssetRegistry::RegisterTexture(std::string_view name, void* view, void* sampler) {
    m_Textures[std::string(name)] = {view, sampler};
}
#endif

AssetTexture AssetRegistry::GetTexture(std::string_view name) const {
    auto it = m_Textures.find(std::string(name));
    if (it != m_Textures.end()) {
        return it->second;
    }
    return {};
}

void AssetRegistry::RegisterFontPath(std::string_view name, std::string_view resolvedPath) {
    m_FontPaths[std::string(name)] = std::string(resolvedPath);
}

void AssetRegistry::RegisterShaderPath(std::string_view name, std::string_view resolvedPath) {
    m_ShaderPaths[std::string(name)] = std::string(resolvedPath);
}

void AssetRegistry::RegisterIconPath(std::string_view name, std::string_view resolvedPath) {
    m_IconPaths[std::string(name)] = std::string(resolvedPath);
}

void AssetRegistry::RegisterIconAtlasRoot(std::string_view resolvedPath) {
    m_IconAtlasRoot = std::string(resolvedPath);
}

void AssetRegistry::RegisterIconMetaPath(std::string_view resolvedPath) {
    m_IconMetaPath = std::string(resolvedPath);
}

std::string AssetRegistry::GetFontPath(std::string_view name) const {
    auto it = m_FontPaths.find(std::string(name));
    return it != m_FontPaths.end() ? it->second : std::string{};
}

std::string AssetRegistry::GetShaderPath(std::string_view name) const {
    auto it = m_ShaderPaths.find(std::string(name));
    return it != m_ShaderPaths.end() ? it->second : std::string{};
}

std::string AssetRegistry::GetIconPath(std::string_view name) const {
    auto it = m_IconPaths.find(std::string(name));
    return it != m_IconPaths.end() ? it->second : std::string{};
}

std::string AssetRegistry::GetIconAtlasRoot() const {
    return m_IconAtlasRoot;
}

std::string AssetRegistry::GetIconMetaPath() const {
    return m_IconMetaPath;
}

std::string AssetRegistry::ResolveAssetPath(const std::vector<std::string>& candidates) {
    for (const auto& path : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(path, ec)) {
            return path;
        }
    }
    return {};
}

AssetLoadResult AssetRegistry::TryLoadAsset(std::string_view name, const std::vector<std::string>& candidates) {
    AssetLoadResult result;
    result.name = std::string(name);
    result.resolvedPath = ResolveAssetPath(candidates);
    result.found = !result.resolvedPath.empty();
    m_LastLoadResults.push_back(result);
    return result;
}

bool AssetRegistry::LoadDefaultEditorAssets() {
    m_LastLoadResults.clear();

    HE_INFO("[Assets] Loading default editor assets...");

    const std::vector<std::pair<std::string, std::vector<std::string>>> fonts = {
        {"Font_UI", {
            "Assets/Fonts/Inter-Regular.ttf",
            "Fonts/Inter-Regular.ttf",
            "../Assets/Fonts/Inter-Regular.ttf",
            "bin/Fonts/Inter-Regular.ttf",
            "../../bin/Fonts/Inter-Regular.ttf"
        }},
    };

    const std::vector<std::pair<std::string, std::vector<std::string>>> shaders = {
        {"UI", {
            "Engine/Shaders/Bytecodes/UI_VS.spv",
            "Assets/Shaders/UI_VS.spv",
            "Shaders/UI_VS.spv",
            "../Assets/Shaders/UI_VS.spv"
        }},
        {"AtmospherePass", {
            "Engine/Shaders/Bytecodes/AtmospherePass_VS.spv",
            "Assets/Shaders/AtmospherePass_VS.spv",
            "Shaders/AtmospherePass_VS.spv"
        }},
        {"VolumetricCloudsPass", {
            "Engine/Shaders/Bytecodes/VolumetricCloudsPass_VS.spv",
            "Assets/Shaders/VolumetricCloudsPass_VS.spv",
            "Shaders/VolumetricCloudsPass_VS.spv"
        }},
        {"CloudTemporalResolve", {
            "Engine/Shaders/Bytecodes/CloudTemporalResolve_VS.spv",
            "Assets/Shaders/CloudTemporalResolve_VS.spv",
            "Shaders/CloudTemporalResolve_VS.spv"
        }},
        {"CloudCompositePass", {
            "Engine/Shaders/Bytecodes/CloudCompositePass_VS.spv",
            "Assets/Shaders/CloudCompositePass_VS.spv",
            "Shaders/CloudCompositePass_VS.spv"
        }},
        {"FogCompositePass", {
            "Engine/Shaders/Bytecodes/FogCompositePass_VS.spv",
            "Assets/Shaders/FogCompositePass_VS.spv",
            "Shaders/FogCompositePass_VS.spv"
        }},
        {"AtmospherePass", {
            "Assets/Shaders/AtmospherePass_VS.spv",
            "Shaders/AtmospherePass_VS.spv"
        }},
        {"EditorGrid", {
            "Engine/Shaders/Bytecodes/EditorGrid_VS.spv",
            "Assets/Shaders/EditorGrid_VS.spv",
            "Shaders/EditorGrid_VS.spv"
        }},
        {"SceneObject", {
            "Engine/Shaders/Bytecodes/SceneObject_VS.spv",
            "Assets/Shaders/SceneObject_VS.spv",
            "Shaders/SceneObject_VS.spv"
        }},
    };

    const std::vector<std::pair<std::string, std::vector<std::string>>> icons = {
        {"Icon_Lucide", {
            "Engine/Content/Icons/icons",
            "Assets/Icons/icons",
            "Icons/icons",
            "../Assets/Icons/icons",
            "../Engine/Content/Icons/icons"
        }},
    };

    const std::vector<std::pair<std::string, std::vector<std::string>>> iconAtlases = {
        {"Icon_AtlasRoot", {
            "Assets/Icons/Atlas",
            "Icons/Atlas",
            "../Assets/Icons/Atlas",
            "Build/Output/Win64/Shipping/Assets/Icons/Atlas"
        }},
    };

    const std::vector<std::pair<std::string, std::vector<std::string>>> iconMeta = {
        {"Icon_Meta", {
            "Assets/Icons/Atlas/icons.weiconmeta",
            "Icons/Atlas/icons.weiconmeta",
            "../Assets/Icons/Atlas/icons.weiconmeta",
            "Build/Output/Win64/Shipping/Assets/Icons/Atlas/icons.weiconmeta"
        }},
    };

    bool allRequiredFound = true;

    for (const auto& [name, paths] : fonts) {
        auto result = TryLoadAsset(name, paths);
        if (result.found) {
            RegisterFontPath(name, result.resolvedPath);
            HE_INFO("[Assets]   Font '" + name + "' -> " + result.resolvedPath);
        } else {
            HE_ERROR("[Assets]   MISSING font '" + name + "'");
            allRequiredFound = false;
        }
    }

    for (const auto& [name, paths] : shaders) {
        auto result = TryLoadAsset(name, paths);
        if (result.found) {
            RegisterShaderPath(name, result.resolvedPath);
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

    for (const auto& [name, paths] : icons) {
        auto result = TryLoadAsset(name, paths);
        if (result.found) {
            RegisterIconPath(name, result.resolvedPath);
            HE_INFO("[Assets]   Icon source '" + name + "' -> " + result.resolvedPath);
        } else {
            HE_INFO("[Assets]   Optional icon source '" + name + "' not found (offline import only)");
        }
    }

    for (const auto& [name, paths] : iconAtlases) {
        auto result = TryLoadAsset(name, paths);
        if (result.found) {
            RegisterIconAtlasRoot(result.resolvedPath);
            HE_INFO("[Assets]   Icon atlas root '" + name + "' -> " + result.resolvedPath);
        } else {
            HE_ERROR("[Assets]   MISSING icon atlas root '" + name + "'");
            allRequiredFound = false;
        }
    }

    for (const auto& [name, paths] : iconMeta) {
        auto result = TryLoadAsset(name, paths);
        if (result.found) {
            RegisterIconMetaPath(result.resolvedPath);
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
    m_Textures.clear();
    m_FontPaths.clear();
    m_ShaderPaths.clear();
    m_IconPaths.clear();
    m_IconAtlasRoot.clear();
    m_IconMetaPath.clear();
    m_LastLoadResults.clear();
}

} // namespace we::core
