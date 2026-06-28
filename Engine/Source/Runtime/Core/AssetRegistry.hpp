#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <volk.h>

namespace we::core {

struct AssetTexture {
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
};

class AssetRegistry {
public:
    static AssetRegistry& Get(); // Ideally injected via ServiceLocator

    void RegisterTexture(std::string_view name, VkImageView view, VkSampler sampler);
    AssetTexture GetTexture(std::string_view name) const;
    void Clear();

private:
    AssetRegistry() = default;
    ~AssetRegistry() = default;

    std::unordered_map<std::string, AssetTexture> m_Textures;
};

} // namespace we::core
