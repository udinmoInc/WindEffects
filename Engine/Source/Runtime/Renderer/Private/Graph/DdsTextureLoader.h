// ==============================================================================
// WindEffects — Renderer — DdsTextureLoader
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace we::runtime::renderer {

struct LoadedDdsTexture {
    we::rhi::RHITextureHandle texture = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureViewHandle view = we::rhi::RHITextureViewHandle::Invalid;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    bool isVolume = false;
};

// Loads DXGI_FORMAT_R8G8B8A8_UNORM DX10 DDS (2D or 3D) into an RHI sampled texture.
[[nodiscard]] std::optional<LoadedDdsTexture> LoadDdsRgba8Texture(
    we::rhi::IRHIDevice& device,
    const std::filesystem::path& path,
    const char* debugName);

void DestroyLoadedDdsTexture(we::rhi::IRHIDevice& device, LoadedDdsTexture& tex);

} // namespace we::runtime::renderer
