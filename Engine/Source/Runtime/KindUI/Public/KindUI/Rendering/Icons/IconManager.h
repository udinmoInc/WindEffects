#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Geometry.h"
#include "KindUI/Core/WindIcon.h"

#include "RHI/Types.h"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace we::runtime::kindui {

class OverlayRenderer;

struct IconDrawInfo {
    we::rhi::RHIDescriptorSetHandle descriptorSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    float uvMin[2] = {0.0f, 0.0f};
    float uvMax[2] = {1.0f, 1.0f};
    float shaderType = 4.0f; // full-color, no mono tint
    uint32_t sizePx = 0;
    bool valid = false;
};

class KINDUI_API IconManager {
public:
    IconManager();
    ~IconManager();

    bool Init(OverlayRenderer* renderer, const std::filesystem::path& windIconsRoot);
    void Shutdown();

    [[nodiscard]] IconDrawInfo ResolveIcon(WindIconRef icon) const;

    [[nodiscard]] bool IsReady() const { return m_Ready; }

private:
    struct CachedTexture {
        we::rhi::RHIDescriptorSetHandle descriptorSet = we::rhi::RHIDescriptorSetHandle::Invalid;
        uint32_t width = 0;
        uint32_t height = 0;
        bool ready = false;
    };

    [[nodiscard]] std::filesystem::path AssetPathFor(WindIconRef icon) const;
    [[nodiscard]] std::string CacheKey(WindIconRef icon) const;
    CachedTexture* LoadTexture(WindIconRef icon) const;
    void DestroyTexture(CachedTexture& texture) const;

    OverlayRenderer* m_Renderer = nullptr;
    std::filesystem::path m_WindIconsRoot;
    bool m_Ready = false;

    mutable std::mutex m_Mutex;
    mutable std::unordered_map<std::string, CachedTexture> m_Textures;
};

} // namespace we::runtime::kindui
