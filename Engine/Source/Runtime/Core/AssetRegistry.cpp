#include "AssetRegistry.hpp"

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

void AssetRegistry::Clear() {
    m_Textures.clear();
}

} // namespace we::core
