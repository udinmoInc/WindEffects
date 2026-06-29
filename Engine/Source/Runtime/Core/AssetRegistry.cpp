#include "AssetRegistry.hpp"
#include "Core/Logger.hpp"

#include <fstream>
#include <filesystem>

namespace we::core {

AssetRegistry& AssetRegistry::Get() {
    static AssetRegistry instance;
    return instance;
}

void AssetRegistry::RegisterTexture(std::string_view name, VkImageView view, VkSampler sampler) {
    m_Textures[std::string(name)] = {view, sampler};
}

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
        {"Font_Icons", {
            "Assets/Fonts/codicon.ttf",
            "Fonts/codicon.ttf",
            "../Assets/Fonts/codicon.ttf",
            "bin/Fonts/codicon.ttf",
            "../../bin/Fonts/codicon.ttf"
        }},
    };

    const std::vector<std::pair<std::string, std::vector<std::string>>> shaders = {
        {"Shader_UI_Vert", {
            "Assets/Shaders/UI_VS.spv",
            "Shaders/UI_VS.spv",
            "../Assets/Shaders/UI_VS.spv"
        }},
        {"Shader_UI_Frag", {
            "Assets/Shaders/UI_PS.spv",
            "Shaders/UI_PS.spv",
            "../Assets/Shaders/UI_PS.spv"
        }},
        {"Shader_Grid_Vert", {
            "Assets/Shaders/Grid_VS.spv",
            "Shaders/Grid_VS.spv"
        }},
        {"Shader_Grid_Frag", {
            "Assets/Shaders/Grid_PS.spv",
            "Shaders/Grid_PS.spv"
        }},
        {"Shader_EditorBackground_Vert", {
            "Assets/Shaders/EditorBackground_VS.spv",
            "Shaders/EditorBackground_VS.spv"
        }},
        {"Shader_EditorBackground_Frag", {
            "Assets/Shaders/EditorBackground_PS.spv",
            "Shaders/EditorBackground_PS.spv"
        }},
        {"Shader_SceneObject_Vert", {
            "Assets/Shaders/SceneObject_VS.spv",
            "Shaders/SceneObject_VS.spv"
        }},
        {"Shader_SceneObject_Frag", {
            "Assets/Shaders/SceneObject_PS.spv",
            "Shaders/SceneObject_PS.spv"
        }},
    };

    const std::vector<std::pair<std::string, std::vector<std::string>>> icons = {
        {"Icon_Atlas", {
            "Assets/Fonts/codicon.ttf",
            "Assets/Icons/icons",
            "Icons/icons"
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
            const bool required = (name == "Shader_UI_Vert" || name == "Shader_UI_Frag");
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
            HE_ERROR("[Assets]   MISSING icon source '" + name + "'");
            allRequiredFound = false;
        }
    }

    HE_INFO("[Assets] Theme loaded (in-memory defaults via Theme::Get())");
    HE_INFO("[Assets] Default asset load " + std::string(allRequiredFound ? "succeeded" : "FAILED"));
    return allRequiredFound;
}

void AssetRegistry::Clear() {
    m_Textures.clear();
    m_FontPaths.clear();
    m_ShaderPaths.clear();
    m_IconPaths.clear();
    m_LastLoadResults.clear();
}

} // namespace we::core
