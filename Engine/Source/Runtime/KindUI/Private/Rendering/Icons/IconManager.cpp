#include "KindUI/Rendering/Icons/IconManager.h"

#include "Icons/Assets/PngLoader.h"
#include "Core/Logger.h"
#include "KindUI/Rendering/OverlayRenderer.h"

#include <format>

namespace we::runtime::kindui {

IconManager::IconManager() = default;

IconManager::~IconManager()
{
    Shutdown();
}

bool IconManager::Init(OverlayRenderer* renderer, const std::filesystem::path& windIconsRoot)
{
    m_Renderer = renderer;
    m_WindIconsRoot = windIconsRoot;
    m_Ready = renderer != nullptr && !windIconsRoot.empty() && std::filesystem::is_directory(windIconsRoot);
    if (!m_Ready) {
        HE_ERROR("[Icons] WindIcons root not found: " + windIconsRoot.string());
        return false;
    }
    HE_INFO("[Icons] WindIcons root: " + windIconsRoot.string());
    return true;
}

void IconManager::Shutdown()
{
    std::scoped_lock lock(m_Mutex);
    for (auto& [key, texture] : m_Textures) {
        (void)key;
        DestroyTexture(texture);
    }
    m_Textures.clear();
    m_Renderer = nullptr;
    m_Ready = false;
}

std::string IconManager::CacheKey(WindIconRef icon) const
{
    return std::string(icon.stem) + "_" + std::to_string(icon.sizePx);
}

std::filesystem::path IconManager::AssetPathFor(WindIconRef icon) const
{
    return m_WindIconsRoot / (CacheKey(icon) + ".png");
}

void IconManager::DestroyTexture(CachedTexture& texture) const
{
    if (m_Renderer && texture.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid) {
        m_Renderer->UnregisterTexture(texture.descriptorSet);
    }
    texture = {};
}

IconManager::CachedTexture* IconManager::LoadTexture(WindIconRef icon) const
{
    if (!m_Renderer || !icon.IsValid()) {
        return nullptr;
    }

    const std::string key = CacheKey(icon);
    {
        std::scoped_lock lock(m_Mutex);
        auto it = m_Textures.find(key);
        if (it != m_Textures.end()) {
            return it->second.ready ? &it->second : nullptr;
        }
    }

    const auto path = AssetPathFor(icon);
    if (!std::filesystem::exists(path)) {
        return nullptr;
    }

    std::vector<uint8_t> rgba;
    uint32_t width = 0;
    uint32_t height = 0;
    if (!we::runtime::icons::LoadPngRgba(path, rgba, width, height)) {
        HE_ERROR("[Icons] Failed to decode WindIcon: " + path.string());
        return nullptr;
    }

    CachedTexture uploaded{};
    uploaded.width = width;
    uploaded.height = height;
    uploaded.descriptorSet = m_Renderer->UploadRgbaTexture(width, height, rgba, false);
    if (uploaded.descriptorSet == we::rhi::RHIDescriptorSetHandle::Invalid) {
        HE_ERROR("[Icons] Failed to upload WindIcon: " + path.string());
        return nullptr;
    }
    uploaded.ready = true;

    std::scoped_lock lock(m_Mutex);
    auto [it, inserted] = m_Textures.emplace(key, uploaded);
    if (!inserted) {
        DestroyTexture(uploaded);
        return it->second.ready ? &it->second : nullptr;
    }
    return &it->second;
}

IconDrawInfo IconManager::ResolveIcon(WindIconRef icon) const
{
    IconDrawInfo info;
    if (!m_Ready || !icon.IsValid()) {
        return info;
    }

    CachedTexture* texture = LoadTexture(icon);
    if (!texture || !texture->ready) {
        return info;
    }

    info.descriptorSet = texture->descriptorSet;
    info.uvMin[0] = 0.0f;
    info.uvMin[1] = 0.0f;
    info.uvMax[0] = 1.0f;
    info.uvMax[1] = 1.0f;
    info.shaderType = 4.0f;
    info.sizePx = icon.sizePx;
    info.valid = true;
    return info;
}

} // namespace we::runtime::kindui
