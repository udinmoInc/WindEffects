// ==============================================================================
// WindEffects — Core — AssetRegistry
// Internal implementation for the Core module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Core/AssetRegistry.h"
#include "Core/AssetCatalog.h"
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

    HE_INFO("[Assets] Loading editor assets from dynamic catalog...");

    const auto catalog = we::runtime::core::AssetCatalogService::Load();
    const auto& paths = PathService::Get();

    bool allRequiredFound = true;

    for (const auto& entry : catalog.fonts) {
        auto result = TryLoadAsset(entry.name, PathsToStrings(paths.FontCandidates(entry.file)));
        if (result.found) {
            m_FontPaths[entry.name] = result.resolvedPath;
            HE_INFO("[Assets]   Font '" + entry.name + "' -> " + result.resolvedPath);
        } else if (entry.required) {
            HE_ERROR("[Assets]   MISSING font '" + entry.name + "'");
            allRequiredFound = false;
        } else {
            HE_INFO("[Assets]   Optional font '" + entry.name + "' not found");
        }
    }

    for (const auto& entry : catalog.shaders) {
        auto result = TryLoadAsset(entry.name, PathsToStrings(paths.ShaderBytecodeCandidates(entry.file)));
        if (result.found) {
            m_ShaderPaths[entry.name] = result.resolvedPath;
            HE_INFO("[Assets]   Shader '" + entry.name + "' -> " + result.resolvedPath);
        } else if (entry.required) {
            HE_ERROR("[Assets]   MISSING required shader '" + entry.name + "'");
            allRequiredFound = false;
        } else {
            HE_INFO("[Assets]   Optional shader '" + entry.name + "' not found (may compile later)");
        }
    }

    for (const auto& entry : catalog.icons) {
        auto result = TryLoadAsset(entry.name, PathsToStrings(paths.IconCandidates(entry.file)));
        if (result.found) {
            m_IconPaths[entry.name] = result.resolvedPath;
            HE_INFO("[Assets]   Icon source '" + entry.name + "' -> " + result.resolvedPath);
        } else {
            HE_INFO("[Assets]   Optional icon source '" + entry.name + "' not found (offline import only)");
        }
    }

    {
        auto result = TryLoadAsset(
            catalog.iconAtlasRoot.name,
            PathsToStrings(paths.IconCandidates(catalog.iconAtlasRoot.file)));
        if (result.found) {
            m_IconAtlasRoot = result.resolvedPath;
            HE_INFO("[Assets]   Icon atlas root '" + catalog.iconAtlasRoot.name + "' -> " + result.resolvedPath);
        } else {
            HE_INFO("[Assets]   Optional icon atlas root not found (will use fallback)");
        }
    }

    {
        auto result = TryLoadAsset(
            catalog.iconMeta.name,
            PathsToStrings(paths.IconCandidates(std::filesystem::path(catalog.iconMeta.file))));
        if (result.found) {
            m_IconMetaPath = result.resolvedPath;
            HE_INFO("[Assets]   Icon meta '" + catalog.iconMeta.name + "' -> " + result.resolvedPath);
        } else {
            HE_INFO("[Assets]   Optional icon meta not found (will use fallback)");
        }
    }

    const auto themeName = we::runtime::core::AssetCatalogService::GetActiveThemeName(catalog);
    HE_INFO("[Assets] Active theme resolver -> " + themeName);
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
