#pragma once

#include "Core/Export.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace we::core {

// Opaque GPU texture bindings — NOT Vulkan/DX/Metal types.
// Renderer/RHI map these to RHITextureViewHandle / RHISamplerHandle.
struct AssetTexture {
    uint64_t view = 0;
    uint64_t sampler = 0;
};

struct AssetLoadResult {
    std::string name;
    std::string resolvedPath;
    bool found = false;
};

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

class CORE_API AssetRegistry {
public:
    static AssetRegistry& Get();

    void RegisterTexture(std::string_view name, uint64_t view, uint64_t sampler);
    AssetTexture GetTexture(std::string_view name) const;

    void RegisterFontPath(std::string_view name, std::string_view resolvedPath);
    void RegisterShaderPath(std::string_view name, std::string_view resolvedPath);
    void RegisterIconPath(std::string_view name, std::string_view resolvedPath);
    void RegisterIconAtlasRoot(std::string_view resolvedPath);
    void RegisterIconMetaPath(std::string_view resolvedPath);

    std::string GetFontPath(std::string_view name) const;
    std::string GetShaderPath(std::string_view name) const;
    std::string GetIconPath(std::string_view name) const;
    std::string GetIconAtlasRoot() const;
    std::string GetIconMetaPath() const;

    bool LoadDefaultEditorAssets();

    const std::vector<AssetLoadResult>& GetLastLoadResults() const { return m_LastLoadResults; }

    void Clear();

    static std::string ResolveAssetPath(const std::vector<std::string>& candidates);

private:
    AssetRegistry() = default;
    ~AssetRegistry() = default;

    AssetLoadResult TryLoadAsset(std::string_view name, const std::vector<std::string>& candidates);

    std::unordered_map<std::string, AssetTexture> m_Textures;
    std::unordered_map<std::string, std::string> m_FontPaths;
    std::unordered_map<std::string, std::string> m_ShaderPaths;
    std::unordered_map<std::string, std::string> m_IconPaths;
    std::string m_IconAtlasRoot;
    std::string m_IconMetaPath;
    std::vector<AssetLoadResult> m_LastLoadResults;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace we::core
