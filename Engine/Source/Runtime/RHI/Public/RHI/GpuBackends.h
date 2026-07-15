#pragma once

#include "RHI/Desc.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>
#include <span>
#include <vector>

namespace we::rhi {

// Backend-agnostic UI draw CPU types. GPU recording uses IRHI only.

struct TextureBind {
    RHITextureViewHandle view = RHITextureViewHandle::Invalid;
    RHISamplerHandle sampler = RHISamplerHandle::Invalid;
    RHIDescriptorSetHandle set = RHIDescriptorSetHandle::Invalid;
};

struct UIVertex {
    float position[2]{};
    float uv[2]{};
    float color[4]{1, 1, 1, 1};
    float sdfRect[4]{};
    float sdfParams[4]{};
};

struct UIDrawBatch {
    RHIDescriptorSetHandle texture = RHIDescriptorSetHandle::Invalid;
    uint32_t indexCount = 0;
    uint32_t firstIndex = 0;
    int32_t vertexOffset = 0;
    float scissor[4]{};
    uint32_t stencilRef = 0;
    bool isText = false;
    uint32_t atlasWidth = 0;
    uint32_t atlasHeight = 0;
    float msdfPixelRange = 4.0f;
};

struct UIDrawList {
    std::vector<UIVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<UIDrawBatch> batches;
    uint32_t targetWidth = 0;
    uint32_t targetHeight = 0;
};

struct FramePresentParams {
    RHITextureHandle colorTarget = RHITextureHandle::Invalid;
    RHITextureViewHandle targetView = RHITextureViewHandle::Invalid;
    Extent2D extent{};
    uint32_t imageIndex = 0;
    IRHICommandList* commandList = nullptr;
};

} // namespace we::rhi
