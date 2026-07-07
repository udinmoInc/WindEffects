#pragma once

#include "Core/Export.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#if WE_HAS_VULKAN
#include <volk.h>
#endif

namespace we::core {

#if WE_HAS_VULKAN
struct AssetTexture {
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
};
#else
struct AssetTexture {
    void* view = nullptr;
    void* sampler = nullptr;
};
#endif

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

#if WE_HAS_VULKAN
    void RegisterTexture(std::string_view name, VkImageView view, VkSampler sampler);
#else
    void RegisterTexture(std::string_view name, void* view, void* sampler);
#endif
    AssetTexture GetTexture(std::string_view name) const;

    void RegisterFontPath(std::string_view name, std::string_view resolvedPath);
    void RegisterShaderPath(std::string_view name, std::string_view resolvedPath);
    void RegisterIconPath(std::string_view name, std::string_view resolvedPath);

    std::string GetFontPath(std::string_view name) const;
    std::string GetShaderPath(std::string_view name) const;
    std::string GetIconPath(std::string_view name) const;

    /// Verifies default editor assets on disk and registers their resolved paths.
    /// Returns false if any required asset is missing.
    bool LoadDefaultEditorAssets();

    const std::vector<AssetLoadResult>& GetLastLoadResults() const { return m_LastLoadResults; }

    void Clear();

private:
    AssetRegistry() = default;
    ~AssetRegistry() = default;

    static std::string ResolveAssetPath(const std::vector<std::string>& candidates);
    AssetLoadResult TryLoadAsset(std::string_view name, const std::vector<std::string>& candidates);

    std::unordered_map<std::string, AssetTexture> m_Textures;
    std::unordered_map<std::string, std::string> m_FontPaths;
    std::unordered_map<std::string, std::string> m_ShaderPaths;
    std::unordered_map<std::string, std::string> m_IconPaths;
    std::vector<AssetLoadResult> m_LastLoadResults;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace we::core
