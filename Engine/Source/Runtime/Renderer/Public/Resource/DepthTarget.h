#pragma once

#include "Renderer/Export.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>

namespace we::runtime::renderer {

class RENDERER_API DepthTarget {
public:
    DepthTarget() = default;
    ~DepthTarget();

    DepthTarget(const DepthTarget&) = delete;
    DepthTarget& operator=(const DepthTarget&) = delete;

    void Init(we::rhi::IRHIDevice* device, uint32_t width, uint32_t height);
    void Shutdown();
    void Resize(uint32_t width, uint32_t height);

    [[nodiscard]] we::rhi::RHITextureHandle GetTexture() const { return m_Texture; }
    [[nodiscard]] we::rhi::Format GetFormat() const { return m_Format; }
    [[nodiscard]] uint32_t GetWidth() const { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const { return m_Height; }
    [[nodiscard]] bool IsValid() const { return m_Texture != we::rhi::RHITextureHandle::Invalid; }

private:
    void CreateResources();

    we::rhi::IRHIDevice* m_Device = nullptr;
    we::rhi::RHITextureHandle m_Texture = we::rhi::RHITextureHandle::Invalid;
    we::rhi::Format m_Format = we::rhi::Format::D32_SFLOAT;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
};

} // namespace we::runtime::renderer
