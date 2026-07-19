#pragma once

#include "KindUI/Export.h"

#include "RHI/Types.h"

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <vector>

namespace we::runtime::kindui {

class OverlayRenderer;

struct AtlasGpuResource {
    uint32_t tierPx = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    we::rhi::RHIDescriptorSetHandle descriptorSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    bool ready = false;
};

/// GPU atlas pages for discovered tier sizes (no fixed compile-time tier list).
class KINDUI_API AtlasManager {
public:
    AtlasManager() = default;
    ~AtlasManager();

    bool Init(OverlayRenderer* renderer);
    void Shutdown();

    bool LoadTierFromFile(uint32_t tierPx, const std::filesystem::path& atlasPath);
    bool EnsureTierLoaded(uint32_t tierPx, const std::filesystem::path& atlasPath);

    [[nodiscard]] const AtlasGpuResource* GetTier(uint32_t tierPx) const;
    [[nodiscard]] bool IsTierReady(uint32_t tierPx) const;
    void WaitDeviceIdle() const;

private:
    [[nodiscard]] int FindTierIndex(uint32_t tierPx) const;
    bool UploadAtlasPage(AtlasGpuResource& tier, const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height);
    void DestroyTier(AtlasGpuResource& tier);

    OverlayRenderer* m_Renderer = nullptr;
    std::vector<AtlasGpuResource> m_Tiers;
    std::vector<std::filesystem::path> m_TierPaths;
    mutable std::mutex m_Mutex;
};

} // namespace we::runtime::kindui
