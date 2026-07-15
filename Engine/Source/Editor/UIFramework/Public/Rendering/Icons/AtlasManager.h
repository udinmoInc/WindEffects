#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include "Rendering/IconMetrics.h"
#include "RHI/Types.h"

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <vector>

namespace WindEffects::Editor::UI {

class OverlayRenderer;

struct AtlasGpuResource {
    uint32_t tierPx = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    we::rhi::RHIDescriptorSetHandle descriptorSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    bool ready = false;
};

class UIFRAMEWORK_API AtlasManager {
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
    bool UploadAtlasPage(AtlasGpuResource& tier, const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height);
    void DestroyTier(AtlasGpuResource& tier);

    OverlayRenderer* m_Renderer = nullptr;
    AtlasGpuResource m_Tiers[IconMetrics::kAtlasTierCount]{};
    std::filesystem::path m_TierPaths[IconMetrics::kAtlasTierCount]{};
    mutable std::mutex m_Mutex;
};

} // namespace WindEffects::Editor::UI
