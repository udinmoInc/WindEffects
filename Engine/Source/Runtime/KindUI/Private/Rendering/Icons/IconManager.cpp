#include "KindUI/Rendering/Icons/IconManager.h"

#include "Icons/Assets/PngLoader.h"
#include "Core/Logger.h"
#include "KindUI/Rendering/OverlayRenderer.h"

#include <algorithm>
#include <cstdint>
#include <format>
#include <vector>

namespace we::runtime::kindui {
namespace {

float ClassifyIconShaderType(const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height) {
    const size_t count = static_cast<size_t>(width) * static_cast<size_t>(height);
    if (rgba.size() < count * 4u) {
        return 0.0f;
    }
    size_t covered = 0;
    size_t chromatic = 0;
    int minL = 255;
    int maxL = 0;
    for (size_t i = 0; i < count; ++i) {
        const uint8_t* p = rgba.data() + i * 4u;
        if (p[3] < 24) {
            continue;
        }
        ++covered;
        const int mx = (std::max)({ p[0], p[1], p[2] });
        const int mn = (std::min)({ p[0], p[1], p[2] });
        if (mx - mn > 30) {
            ++chromatic;
        }
        const int luma = (static_cast<int>(p[0]) + static_cast<int>(p[1]) + static_cast<int>(p[2])) / 3;
        minL = (std::min)(minL, luma);
        maxL = (std::max)(maxL, luma);
    }
    if (covered == 0) {
        return 0.0f;
    }
    // Keep authored RGB when the glyph has accent color or inner dark/light structure.
    if (chromatic * 4u > covered || (maxL - minL) > 48) {
        return 4.0f;
    }
    return 0.0f;
}

} // namespace

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
    const auto path = AssetPathFor(icon);
    if (std::filesystem::exists(path)) {
        const auto sourceWriteTime = std::filesystem::last_write_time(path);
        std::scoped_lock lock(m_Mutex);
        auto it = m_Textures.find(key);
        if (it != m_Textures.end()) {
            if (it->second.ready && it->second.sourceWriteTime == sourceWriteTime) {
                return it->second.ready ? &it->second : nullptr;
            }
            DestroyTexture(it->second);
            m_Textures.erase(it);
        }
    } else {
        std::scoped_lock lock(m_Mutex);
        auto it = m_Textures.find(key);
        if (it != m_Textures.end()) {
            if (it->second.ready) {
                return &it->second;
            }
            return nullptr;
        }
    }

    if (!std::filesystem::exists(path)) {
        return nullptr;
    }

    const auto sourceWriteTime = std::filesystem::last_write_time(path);

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
    uploaded.shaderType = ClassifyIconShaderType(rgba, width, height);
    uploaded.sourceWriteTime = sourceWriteTime;

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
    info.shaderType = texture->shaderType;
    info.sizePx = icon.sizePx;
    info.valid = true;
    return info;
}

} // namespace we::runtime::kindui
