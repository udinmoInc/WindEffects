#pragma once

#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>

namespace we::editor::rendering {

struct OverlayRenderContext {
    we::rhi::IRHICommandList* cmd = nullptr;
    we::rhi::RHITextureViewHandle targetView = we::rhi::RHITextureViewHandle::Invalid;
    we::rhi::Format targetFormat = we::rhi::Format::Unknown;
    we::rhi::Extent2D targetExtent{};
    uint32_t imageIndex = 0;
    int32_t viewportOffsetX = 0;
    int32_t viewportOffsetY = 0;
};

} // namespace we::editor::rendering
